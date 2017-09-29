/*
 * Copyright (C) 2017 Ispirata Srl
 *
 * This file is part of Astarte.
 * Astarte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Astarte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Astarte.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "crypto_p.h"

#include "utils/hemeracommonoperations.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFutureWatcher>

#include <QtCore/QtConcurrentRun>

#include <QtNetwork/QNetworkReply>

// OpenSSL and its crap
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>

namespace Astarte {

class Crypto::Private
{
public:
    Private(Crypto *q) : q(q), pkey(NULL), keystoreAvailable(false) { init_openssl(); }
    virtual ~Private() { cleanup_openssl(); }

    Crypto * const q;

    EVP_PKEY* pkey;
    QByteArray hardwareId;
    bool keystoreAvailable;

    // OpenSSL functions
    void init_openssl();
    void cleanup_openssl();

    static EVP_PKEY* create_rsa_key(RSA *pRSA);
    static int generateKeypair(const QString &privateKeyFile, const QString &publicKeyFile);
    static bool generateCSR(const QString &cn, const QString &privateKeyFile, const QString &csrOutputFile);
    static Hemera::Operation *generateKeypairThreaded(const QString &privateKeyFile, const QString &publicKeyFile);
    Hemera::Operation *generateKeystoreThreaded();

    QByteArray signMessage(const QByteArray &message);
    int sign_it(const void* msg, size_t mlen, QByteArray *signature);

    void loadKeyStore();
};

void Crypto::Private::init_openssl()
{
    if(SSL_library_init())
    {
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_ciphers();
        OpenSSL_add_all_digests();
        RAND_load_file("/dev/urandom", 1024);
    } else {
        qDebug() << "OpenSSL initialization failed!";
        exit(EXIT_FAILURE);
    }
    qDebug() << "OpenSSL initialized correctly!";
}

void Crypto::Private::cleanup_openssl()
{
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(0);
    EVP_cleanup();
}

Hemera::Operation* Crypto::Private::generateKeypairThreaded(const QString& privateKeyFile, const QString& publicKeyFile)
{
    return new ThreadedKeyOperation(privateKeyFile, publicKeyFile);
}

Hemera::Operation* Crypto::Private::generateKeystoreThreaded()
{

    return new ThreadedKeyOperation(QLatin1String(hardwareId), q->pathToPrivateKey(),
                                    q->pathToPublicKey(), q->pathToCertificateRequest());
}

int Crypto::Private::generateKeypair(const QString &privateKeyFile, const QString &publicKeyFile)
{
    qDebug() << "Generating RSA keypair!";

    int iRet = EXIT_SUCCESS;
    EVP_PKEY* pPrivKey = NULL;
    EVP_PKEY* pPubKey  = NULL;
    FILE*     pFile    = NULL;
    RSA *pRSA          = RSA_new();

    // Bignum for key generation
    BIGNUM *e = NULL;
    e = BN_new();
    BN_set_word(e, 65537);

    qDebug() << "Starting generation";
    if (RSA_generate_key_ex(pRSA, 2048, e, NULL) == 1) {
        qDebug() << "Done!";
        pPrivKey = create_rsa_key(pRSA);
        pPubKey  = create_rsa_key(pRSA);
    } else {
        qDebug() << "Key generation failed!";
    }

    /* the big number is no longer used */
    BN_free(e);
    e = NULL;

    if (pPrivKey && pPubKey) {
        /* Save the keys */
        if ((pFile = fopen(privateKeyFile.toStdString().c_str(),"wt"))) {
            if(!PEM_write_PrivateKey(pFile,pPrivKey,NULL, NULL, 0, 0, NULL)) {
                qWarning() << "PEM_write_PrivateKey failed.";
                iRet = EXIT_FAILURE;
            }
            fclose(pFile);
            pFile = NULL;
            if(iRet == EXIT_SUCCESS) {
                if((pFile = fopen(publicKeyFile.toStdString().c_str(),"wt")) && PEM_write_PUBKEY(pFile,pPubKey)) {
                    qDebug() << "Both keys saved.";
                } else {
                    iRet = EXIT_FAILURE;
                }
                if (pFile) {
                    fclose(pFile);
                    pFile = NULL;
                }
            }
        } else {
            qWarning() << "Cannot create \"privkey.pem\".";
            iRet = EXIT_FAILURE;
            if(pFile)
            {
                fclose(pFile);
                pFile = NULL;
            }
        }
    }

    qDebug() << "Keys saved!";

    qDebug() << "Freeing private key";
    if (pPrivKey) {
        EVP_PKEY_free(pPrivKey);
        pPrivKey = NULL;
    }
    qDebug() << "Freeing public key";
    if (pPubKey) {
        // FIXME: This makes everything crash. It's likely freeing the private key causes the
        //        public key to be freed as well, but it needs to be checked.
        //EVP_PKEY_free(pPubKey);
        pPubKey = NULL;
    }
    qDebug() << "Freed!";

    return iRet;
}

bool Crypto::Private::generateCSR(const QString &cn, const QString& privateKeyFile, const QString& csrOutputFile)
{
    qDebug() << "Generating CSR" << csrOutputFile;
    int             ret = 0;

    int             nVersion = 1;

    X509_REQ        *x509_req = NULL;
    X509_NAME       *x509_name = NULL;
    EVP_PKEY        *pKey = NULL;
    BIO             *out = NULL;

    /*const char      *szCountry = "";
    const char      *szProvince = "";
    const char      *szCity = "";*/
    const char      *szOrganization = "Hemera";

    // 1. Load our private key
    FILE *file = fopen(privateKeyFile.toLocal8Bit(), "r");
    pKey = PEM_read_PrivateKey(file, NULL, 0, NULL);
    fclose(file);
    if (!pKey) {
        qWarning() << "Could not get pkey!";
        return false;
    }

    // 2. set version of x509 req
    x509_req = X509_REQ_new();
    ret = X509_REQ_set_version(x509_req, nVersion);
    if (ret != 1) {
        qWarning() << "Could not get X509!";
        goto free_all;
    }

    // 3. set subject of x509 req
    x509_name = X509_REQ_get_subject_name(x509_req);

    /*ret = X509_NAME_add_entry_by_txt(x509_name,"C", MBSTRING_ASC, (const unsigned char*)szCountry, -1, -1, 0);
    if (ret != 1){
        goto free_all;
    }

    ret = X509_NAME_add_entry_by_txt(x509_name,"ST", MBSTRING_ASC, (const unsigned char*)szProvince, -1, -1, 0);
    if (ret != 1){
        goto free_all;
    }

    ret = X509_NAME_add_entry_by_txt(x509_name,"L", MBSTRING_ASC, (const unsigned char*)szCity, -1, -1, 0);
    if (ret != 1){
        goto free_all;
    }*/

    ret = X509_NAME_add_entry_by_txt(x509_name,"O", MBSTRING_ASC, (const unsigned char*)szOrganization, -1, -1, 0);
    if (ret != 1){
        qWarning() << "Could not add entry O!";
        goto free_all;
    }

    ret = X509_NAME_add_entry_by_txt(x509_name,"CN", MBSTRING_ASC, (const unsigned char*)cn.toLocal8Bit().constData(), -1, -1, 0);
    if (ret != 1){
        qWarning() << "Could not add entry CN!";
        goto free_all;
    }

    // 4. set public key of x509 req
    ret = X509_REQ_set_pubkey(x509_req, pKey);
    if (ret != 1){
        qWarning() << "Could not set pubkey!";
        goto free_all;
    }

    // 5. set sign key of x509 req
    ret = X509_REQ_sign(x509_req, pKey, EVP_sha1());    // return x509_req->signature->length
    if (ret <= 0){
        qWarning() << "Could not sign request!";
        goto free_all;
    }

    out = BIO_new_file(csrOutputFile.toLocal8Bit().constData(),"w");
    ret = PEM_write_bio_X509_REQ(out, x509_req);
    qDebug() << "Wrote CSR!" << csrOutputFile.toLocal8Bit().constData() << ret;

    // 6. free
free_all:
    X509_REQ_free(x509_req);
    BIO_free_all(out);

    EVP_PKEY_free(pKey);

    return (ret == 1);
}


EVP_PKEY* Crypto::Private::create_rsa_key(RSA *pRSA)
{
    EVP_PKEY* pKey = NULL;
    pKey = EVP_PKEY_new();
    if(pRSA && pKey && EVP_PKEY_assign_RSA(pKey,pRSA)) {
        /* pKey owns pRSA from now */
        if(RSA_check_key(pRSA) <= 0) {
            qWarning() << "RSA_check_key failed.";
            EVP_PKEY_free(pKey);
            pKey = NULL;
        }
    } else {
        qWarning() << "Something went wrong in RSA key creation.";
        if(pRSA) {
            RSA_free(pRSA);
            pRSA = NULL;
        }
        if(pKey) {
            EVP_PKEY_free(pKey);
            pKey = NULL;
        }
    }
    return pKey;
}

QByteArray Crypto::Private::signMessage(const QByteArray& message)
{
    QByteArray signature;

    int res = sign_it(message.constData(), message.length(), &signature);
    if (res != 1) {
        // aaaaa
    }

    return signature;
}

// COPYPASTE FROM https://wiki.openssl.org/index.php/EVP_Signing_and_Verifying#Signing
int Crypto::Private::sign_it(const void* msg, size_t mlen, QByteArray *signature)
{
    EVP_MD_CTX *mdctx = NULL;
    int ret = 0;
    uchar* sig = NULL;
    size_t slen;

    /* Create the Message Digest Context */
    if(!(mdctx = EVP_MD_CTX_create())) goto err;

    /* Initialise the DigestSign operation - SHA-256 has been selected as the message digest function in this example */
    if(1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey)) goto err;

    /* Call update with the message */
    if(1 != EVP_DigestSignUpdate(mdctx, msg, mlen)) goto err;

    /* Finalise the DigestSign operation */
    /* First call EVP_DigestSignFinal with a NULL sig parameter to obtain the length of the
    * signature. Length is returned in slen */
    if (1 != EVP_DigestSignFinal(mdctx, NULL, &slen)) goto err;
    /* Allocate memory for the signature based on size in slen */
    if (!(sig = (uchar*)OPENSSL_malloc(sizeof(unsigned char) * (slen)))) goto err;
    /* Obtain the signature */
    if (1 != EVP_DigestSignFinal(mdctx, sig, &slen)) goto err;

    // Time to base64 encode
    // NOTE: We have verified that QByteArray's toBase64 actually returns the same value of openssl's
    //       base64 encode.
    *signature = QByteArray::fromRawData(reinterpret_cast<char*>(sig), slen).toBase64();

    /* Success */
    ret = 1;

    err:
    if (ret != 1) {
        /* Do some error handling */
        qWarning() << "Message signing failed with " << ret;
    }

    /* Clean up */
    if(sig) OPENSSL_free(sig);
    if(mdctx) EVP_MD_CTX_destroy(mdctx);

    return ret;
}

void Crypto::Private::loadKeyStore()
{

    bool newAvailable = QFile::exists(q->pathToCertificateRequest()) && QFile::exists(q->pathToPublicKey());
    if (newAvailable && QFile::exists(q->pathToPrivateKey())) {
        if (pkey) {
            // Free the previous key
            EVP_PKEY_free(pkey);
        }

        FILE *file = fopen(q->pathToPrivateKey().toLatin1().constData(), "r");
        pkey = PEM_read_PrivateKey(file, NULL, 0, NULL);
        fclose(file);
        newAvailable = pkey;
    }

    if (newAvailable != keystoreAvailable) {
        keystoreAvailable = newAvailable;

        Q_EMIT q->keyStoreAvailabilityChanged();
    }
}

ThreadedKeyOperation::ThreadedKeyOperation(const QString &privateKeyFile, const QString &publicKeyFile, QObject* parent)
    : Hemera::Operation(parent)
    , m_cn(QString())
    , m_privateKeyFile(privateKeyFile)
    , m_publicKeyFile(publicKeyFile)
    , m_csrFile(QString())
{
}

ThreadedKeyOperation::ThreadedKeyOperation(const QString &cn, const QString &privateKeyFile, const QString &publicKeyFile,
                                           const QString &csrFile, QObject* parent)
    : Hemera::Operation(parent)
    , m_cn(cn)
    , m_privateKeyFile(privateKeyFile)
    , m_publicKeyFile(publicKeyFile)
    , m_csrFile(csrFile)
{
}

ThreadedKeyOperation::~ThreadedKeyOperation()
{
}

void ThreadedKeyOperation::startImpl()
{
    QFuture<int> result = QtConcurrent::run(Astarte::Crypto::Private::generateKeypair, m_privateKeyFile, m_publicKeyFile);
    watcher = new QFutureWatcher<int>(this);
    watcher->setFuture(result);
    connect(watcher, SIGNAL(finished()), this, SLOT(onWatcherFinished()));
}

void ThreadedKeyOperation::onWatcherFinished()
{
    if (watcher->result() == EXIT_SUCCESS) {
        // Do we need to process a CSR?
        if (m_csrFile.isEmpty()) {
            setFinished();
            return;
        }

        QFuture<bool> csrResult = QtConcurrent::run(Astarte::Crypto::Private::generateCSR, m_cn, m_privateKeyFile, m_csrFile);
        csrWatcher = new QFutureWatcher<bool>(this);
        csrWatcher->setFuture(csrResult);
        connect(csrWatcher, SIGNAL(finished()), this, SLOT(onCSRWatcherFinished()));
    } else {
        setFinishedWithError("failed", QString("Generation failed with %1.").arg(watcher->result()));
    }
}

void ThreadedKeyOperation::onCSRWatcherFinished()
{
    if (csrWatcher->result()) {
        setFinished();
    } else {
        setFinishedWithError("failed", QString("CSR Generation failed with %1.").arg(csrWatcher->result()));
    }
}

class CryptoHelper
{
public:
    CryptoHelper() : q(0) {}
    ~CryptoHelper() {
        delete q;
    }
    Crypto *q;
};

Q_GLOBAL_STATIC(CryptoHelper, s_globalCrypto)
QString Crypto::s_cryptoBasePath = QString();

static void cleanup_crypto()
{
    if (s_globalCrypto()->q) {
        s_globalCrypto()->q->deleteLater();
    }
}

Crypto * Crypto::instance()
{
    if (!s_globalCrypto()->q) {
        new Crypto();
    }

    return s_globalCrypto()->q;
}

Crypto::Crypto(QObject *parent)
    : AsyncInitObject(parent)
    , d(new Crypto::Private(this))
{
    Q_ASSERT(!s_globalCrypto()->q);
    s_globalCrypto()->q = this;

    // Add a post routine for correct deletion of QNAM.
    qAddPostRoutine(cleanup_crypto);

    // Does the dir exist?
    QDir dir;
    if (!dir.exists(cryptoBasePath())) {
        // Create
        qDebug() << "Creating Astarte Crypto store directory";
        QDir cryptoDir;
        if (!cryptoDir.mkpath(cryptoBasePath())) {
            qWarning() << "Could not create Crypto store directory! This is most likely due to an installation problem. "
                                       << "Crypto operations on this transport will not work.";
        }
    }
}

Crypto::~Crypto()
{
}

void Crypto::initImpl()
{
    // Get our needed Fingerprints.
    if (d->hardwareId.isEmpty()) {
        setInitError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())", tr("Hardware ID must be set before init!"));
        return;
    }

    // Attempt loading keystore
    d->loadKeyStore();
    setReady();
}


void Crypto::setCryptoBasePath(const QString &basePath)
{
    s_cryptoBasePath = basePath;
}

QByteArray Crypto::sign(const QByteArray& payload, Crypto::AuthenticationDomains domains)
{
    if (domains & DeviceAuthenticationDomain) {
        return d->signMessage(payload);
    }

    qDebug() << "No way I can sign this.";
    return QByteArray("cacca").toBase64();
}

void Crypto::setHardwareId(const QByteArray &hardwareId)
{
    Crypto::instance()->d->hardwareId = hardwareId;
}

bool Crypto::isKeyStoreAvailable() const
{

    return d->keystoreAvailable;
}

Hemera::Operation* Crypto::generateAstarteKeyStore(bool forceGeneration)
{

    if (d->keystoreAvailable && !forceGeneration) {
        return new Hemera::FailureOperation("badrequest", tr("The keystore is already available!"));
    }

    return d->generateKeystoreThreaded();
}

QString Crypto::cryptoBasePath()
{
    return s_cryptoBasePath;
}

QString Crypto::pathToCertificateRequest()
{
    return QString("%1/%2").arg(cryptoBasePath(), "astartekey.csr");
}

QString Crypto::pathToPrivateKey()
{
    return QString("%1/%2").arg(cryptoBasePath(), "astartekey.pem");
}

QString Crypto::pathToPublicKey()
{
    return QString("%1/%2").arg(cryptoBasePath(), "astartekey.pub");
}

}
