#include "httpendpoint.h"
#include "httpendpoint_p.h"

#include "crypto.h"
#include "verifycertificateoperation.h"
#include "mqttclientwrapper.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "utils/hemeracommonoperations.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "endpoint_p.h"

#include "utils/utils.h"

#define RETRY_INTERVAL 15000


namespace Astarte {

PairOperation::PairOperation(HTTPEndpoint *parent)
    : Hemera::Operation(parent)
    , m_endpoint(parent)
{
}

PairOperation::~PairOperation()
{
}

void PairOperation::startImpl()
{
    // Before anything else, we need to check if we have an available keystore.
    if (!Crypto::instance()->isKeyStoreAvailable()) {
        // Let's build one.
        Hemera::Operation *op = Crypto::instance()->generateAstarteKeyStore();
        connect(op, SIGNAL(finished(Hemera::Operation*)), this, SLOT(onGenerationFinished(Hemera::Operation*)));
    } else {
        // Let's just go
        initiatePairing();
    }
}

void PairOperation::onGenerationFinished(Hemera::Operation *op)
{
    if (op->isError()) {
        // That's ugly.
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    initiatePairing();
}

void PairOperation::initiatePairing()
{
    // FIXME: This should be done using Global configuration!!
    QSettings settings(QString("%1/endpoint_crypto.conf").arg(m_endpoint->pathToAstarteEndpointConfiguration(m_endpoint->d_func()->endpointName)),
                       QSettings::IniFormat);
    if (settings.value(QLatin1String("apiKey")).toString().isEmpty()) {
        performFakeAgentPairing();
    } else {
        performPairing();
    }
}

void PairOperation::performFakeAgentPairing()
{
    qWarning() << "Fake agent pairing!";
    QString jsonDocument = QString("{\"hwId\":\"%1\"}").arg(QString(m_endpoint->d_func()->hardwareId));

    QNetworkReply *r = m_endpoint->sendRequest(QLatin1String("/devices/apikeysFromDevice"), jsonDocument.toUtf8(), Crypto::CustomerAuthenticationDomain);

    qDebug() << "I'm sending: " << jsonDocument;

    connect(r, SIGNAL(finished()), this, SLOT(onDeviceIDPayloadFinished()));
}

void PairOperation::onDeviceIDPayloadFinished()
{
    QNetworkReply *r = qobject_cast<QNetworkReply*>(sender());
    if (r->error() != QNetworkReply::NoError) {
        qWarning() << "Pairing error! Error: " << r->error();
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())", r->errorString());
        r->deleteLater();
        return;
    }

    rapidjson::Document doc;
    doc.Parse(r->readAll());
    r->deleteLater();
    qDebug() << "Got the ok!";
    if (!doc.IsObject()) {
        qWarning() << "Parsing pairing result error!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", QLatin1String("Parsing pairing result error!"));
        return;
    }

    qDebug() << "Payload is ";

    if (!doc.HasMember("apiKey")) {
        qWarning() << "Missing apiKey in the pairing routine!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())",
                             QLatin1String("Missing apiKey in the pairing routine!"));
        return;
    }

    // Ok, we need to write the files now.
    {
        QSettings settings(QString("%1/endpoint_crypto.conf").arg(m_endpoint->pathToAstarteEndpointConfiguration(m_endpoint->d_func()->endpointName)),
                           QSettings::IniFormat);
        settings.setValue(QLatin1String("apiKey"), doc["apiKey"].GetString());
    }

    // That's all, folks!
    performPairing();
}

void PairOperation::performPairing()
{
    QFile csr(Crypto::instance()->pathToCertificateRequest());
    if (!csr.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open CSR for reading! Aborting.";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())", QLatin1String("Could not open CSR for reading! Aborting."));
    }

    QByteArray deviceIDPayload = csr.readAll();
    QNetworkReply *r = m_endpoint->sendRequest(QLatin1String("/pairing"), deviceIDPayload, Crypto::DeviceAuthenticationDomain);

    qDebug() << "I'm sending: " << deviceIDPayload.constData();

    connect(r, SIGNAL(finished()), this, SLOT(onPairingReply()));
}

void PairOperation::onPairingReply()
{
    QNetworkReply *r = qobject_cast<QNetworkReply*>(sender());
    if (r->error() != QNetworkReply::NoError) {
        qWarning() << "Pairing error!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())", r->errorString());
        r->deleteLater();
        return;
    }

    rapidjson::Document doc;
    doc.Parse(r->readAll());
    r->deleteLater();
    qDebug() << "Got the ok!";
    if (!doc.IsObject()) {
        qWarning() << "Parsing pairing result error!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", QLatin1String("Parsing pairing result error!"));
        return;
    }

    qDebug() << "Payload is ";

    if (!doc.HasMember("clientCrt")) {
        qWarning() << "Missing certificate in the pairing routine!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", QLatin1String("Missing certificate in the pairing routine!"));
        return;
    }

    // Ok, we need to write the files now.
    {
        QFile generatedCertificate(QString("%1/mqtt_broker.crt").arg(m_endpoint->pathToAstarteEndpointConfiguration(m_endpoint->d_func()->endpointName)));
        if (!generatedCertificate.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Could not write certificate!";
            setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", generatedCertificate.errorString());
            return;
        }
        generatedCertificate.write(doc["clientCrt"].GetString());
        generatedCertificate.flush();
        generatedCertificate.close();
    }
    {
        QSettings settings(QString("%1/mqtt_broker.conf").arg(m_endpoint->pathToAstarteEndpointConfiguration(m_endpoint->d_func()->endpointName)),
                           QSettings::IniFormat);
    }

    // That's all, folks!
    setFinished();
}

void HTTPEndpointPrivate::connectToEndpoint()
{
    QUrl infoEndpoint = endpoint;
    infoEndpoint.setPath(endpoint.path() + QLatin1String("/info"));
    QNetworkRequest req(infoEndpoint);
    req.setSslConfiguration(sslConfiguration);
    req.setRawHeader("Authorization", agentKey);
    req.setRawHeader("X-Astarte-Transport-Provider", "Hemera");
    req.setRawHeader("X-Astarte-Transport-Version", QString("%1.%2.%3")
                                                     .arg(Utils::majorVersion())
                                                     .arg(Utils::minorVersion())
                                                     .arg(Utils::releaseVersion())
                                                     .toLatin1());
    QNetworkReply *reply = nam->get(req);
    qDebug() << "Connecting to our endpoint! " << infoEndpoint;

    Q_Q(HTTPEndpoint);
    QObject::connect(reply, SIGNAL(finished()), q, SLOT(onConnectionEstablished()));
}

void HTTPEndpointPrivate::onConnectionEstablished()
{
    Q_Q(HTTPEndpoint);
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(q->sender());

    if (reply->error() != QNetworkReply::NoError) {
        int retryInterval = Utils::randomizedInterval(RETRY_INTERVAL, 1.0);
        qWarning() << "Error while connecting! Retrying in " << (retryInterval / 1000) << " seconds. error: " << reply->error();

        // We never give up. If we couldn't connect, we reschedule this in 15 seconds.
        QTimer::singleShot(retryInterval, q, SLOT(connectToEndpoint()));
        reply->deleteLater();
        return;
    }

    rapidjson::Document doc;
    doc.Parse(reply->readAll());
    reply->deleteLater();
    if (!doc.IsObject()) {
        return;
    }

    qDebug() << "Connected! ";

    endpointVersion = doc["version"].GetString();

    // Get configuration
    QSettings settings(QString("%1/mqtt_broker.conf").arg(q->pathToAstarteEndpointConfiguration(endpointName)),
                       QSettings::IniFormat);
    mqttBroker = QUrl::fromUserInput(doc["url"].GetString());

    if (Crypto::instance()->isReady() || Crypto::instance()->hasInitError()) {
        processCryptoStatus();
    } else {
        Crypto::instance()->setHardwareId(hardwareId);
        QObject::connect(Crypto::instance()->init(), SIGNAL(finished(Hemera::Operation*)), q, SLOT(processCryptoStatus()));
    }
}

void HTTPEndpointPrivate::processCryptoStatus()
{
    Q_Q(HTTPEndpoint);
    if (Crypto::instance()->hasInitError()) {
        qWarning() << "Could not initialize signature system!!";
        if (!q->isReady()) {
            q->setInitError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())",
                            HTTPEndpoint::tr("Could not initialize signature system!"));
        }
        return;
    }

    qDebug() << "Signature system initialized correctly!";

    if (!q->isReady()) {
        q->setReady();
    }
}


HTTPEndpoint::HTTPEndpoint(const QString &configurationFile, const QString &persistencyDir, const QUrl& endpoint, const QByteArray &hardwareId, const QSslConfiguration &sslConfiguration, QObject* parent)
    : Endpoint(*new HTTPEndpointPrivate(this), endpoint, parent)
{
    Q_D(HTTPEndpoint);
    d->nam = new QNetworkAccessManager(this);
    d->sslConfiguration = sslConfiguration;
    d->configurationFile = configurationFile;
    d->persistencyDir = persistencyDir;
    d->hardwareId = hardwareId;
    Crypto::setCryptoBasePath(QString("%1/crypto").arg(persistencyDir));

    d->endpointName = endpoint.host();
}

HTTPEndpoint::~HTTPEndpoint()
{
}

QUrl HTTPEndpoint::endpoint() const
{
    Q_D(const HTTPEndpoint);
    return d->endpoint;
}

void HTTPEndpoint::initImpl()
{
    Q_D(HTTPEndpoint);

    QSettings settings(d->configurationFile, QSettings::IniFormat);
    settings.beginGroup(QLatin1String("AstarteTransport")); {
        d->agentKey = settings.value(QLatin1String("agentKey")).toString().toLatin1();
        d->brokerCa = settings.value(QLatin1String("brokerCa"), QLatin1String("/etc/ssl/certs/ca-certificates.crt")).toString();
        d->ignoreSslErrors = settings.value(QLatin1String("ignoreSslErrors"), false).toBool();
        if (settings.contains(QLatin1String("pairingCa"))) {
            d->sslConfiguration.setCaCertificates(QSslCertificate::fromPath(settings.value(QLatin1String("pairingCa")).toString()));
        }
    } settings.endGroup();

    // Verify the configuration directory is up.
    QDir dir;
    if (!dir.exists(pathToAstarteEndpointConfiguration(d->endpointName))) {
        // Create
        qDebug() << "Creating Astarte endpoint configuration directory for " << d->endpointName;
        if (!dir.mkpath(pathToAstarteEndpointConfiguration(d->endpointName))) {
            qWarning() << "Could not create configuration directory!";
        }
    }

    // Let's connect to our endpoint, shall we?
    d->connectToEndpoint();

    connect(d->nam, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(ignoreSslErrors(QNetworkReply*,QList<QSslError>)));
}

void HTTPEndpoint::ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_D(const HTTPEndpoint);
    qWarning() << "SslErrors: " << errors;
    if (d->ignoreSslErrors) {
        reply->ignoreSslErrors(errors);
    }
}

QString HTTPEndpoint::endpointConfigurationBasePath() const
{
    Q_D(const HTTPEndpoint);
    return QString("%1/endpoint/").arg(d->persistencyDir);
}

QString HTTPEndpoint::pathToAstarteEndpointConfiguration(const QString &endpointName) const
{
    return QString("%1%2").arg(endpointConfigurationBasePath(), endpointName);
}

QNetworkReply *HTTPEndpoint::sendRequest(const QString& relativeEndpoint, const QByteArray& payload, Crypto::AuthenticationDomain authenticationDomain)
{
    Q_D(const HTTPEndpoint);
    // Build the endpoint
    QUrl target = d->endpoint;
    target.setPath(d->endpoint.path() + relativeEndpoint);

    QNetworkRequest req(target);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("application/json"));
    req.setSslConfiguration(d->sslConfiguration);

    qWarning() << "La richiesta lo zio" << relativeEndpoint << payload << authenticationDomain;
    // Authentication?
    if (authenticationDomain == Crypto::DeviceAuthenticationDomain) {
        // FIXME: This should be done using Global configuration!!
        QSettings settings(QString("%1/endpoint_crypto.conf").arg(pathToAstarteEndpointConfiguration(d->endpointName)),
                           QSettings::IniFormat);
        req.setRawHeader("X-API-Key", settings.value(QLatin1String("apiKey")).toString().toLatin1());
        req.setRawHeader("X-Hardware-ID", d->hardwareId);
        req.setRawHeader("X-Astarte-Transport-Provider", "Hemera");
        req.setRawHeader("X-Astarte-Transport-Version", QString("%1.%2.%3")
                                                     .arg(Utils::majorVersion())
                                                     .arg(Utils::minorVersion())
                                                     .arg(Utils::releaseVersion())
                                                     .toLatin1());
        qWarning() << "Lozio";
        qWarning() << "Setting X-API-Key:" << settings.value(QLatin1String("apiKey")).toString();
    } else if (authenticationDomain == Crypto::CustomerAuthenticationDomain) {
        qWarning() << "Authorization lo zio";
        req.setRawHeader("Authorization", d->agentKey);
        req.setRawHeader("X-Astarte-Transport-Provider", "Hemera");
        req.setRawHeader("X-Astarte-Transport-Version", QString("%1.%2.%3")
                                                     .arg(Utils::majorVersion())
                                                     .arg(Utils::minorVersion())
                                                     .arg(Utils::releaseVersion())
                                                     .toLatin1());
    }

    return d->nam->post(req, payload);
}

Hemera::Operation* HTTPEndpoint::pair(bool force)
{
    if (!force && isPaired()) {
        return new Hemera::FailureOperation("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())",
                                            tr("This device is already paired to this Astarte endpoint"));
    }

    return new PairOperation(this);
}

Hemera::Operation* HTTPEndpoint::verifyCertificate()
{
    Q_D(const HTTPEndpoint);
    QFile cert(QString("%1/mqtt_broker.crt").arg(pathToAstarteEndpointConfiguration(d->endpointName)));
    return new VerifyCertificateOperation(cert, this);
}

MQTTClientWrapper *HTTPEndpoint::createMqttClientWrapper()
{
    if (mqttBrokerUrl().isEmpty()) {
        return 0;
    }

    Q_D(const HTTPEndpoint);

    // Create the client ID
    QList < QSslCertificate > certificates = QSslCertificate::fromPath(QString("%1/mqtt_broker.crt")
                                                    .arg(pathToAstarteEndpointConfiguration(d->endpointName)), QSsl::Pem);
    if (certificates.size() != 1) {
        qWarning() << "Could not retrieve device certificate!";
        return 0;
    }

    // FIXME: Qt5 vs Qt4 API change!! Missing first()
    QByteArray customerId = certificates.first().subjectInfo(QSslCertificate::CommonName).toLatin1();

    MQTTClientWrapper *c = new MQTTClientWrapper(mqttBrokerUrl(), customerId, this);

    c->setCleanSession(false);
    c->setPublishQoS(MQTTClientWrapper::AtMostOnceQoS);
    c->setSubscribeQoS(MQTTClientWrapper::AtMostOnceQoS);
    c->setKeepAlive(300);
    c->setIgnoreSslErrors(d->ignoreSslErrors);

    // SSL
    c->setMutualSSLAuthentication(d->brokerCa, Crypto::pathToPrivateKey(),
                                  QString("%1/mqtt_broker.crt").arg(pathToAstarteEndpointConfiguration(d->endpointName)));

    QSettings settings(QString("%1/mqtt_broker.conf").arg(pathToAstarteEndpointConfiguration(d->endpointName)),
                       QSettings::IniFormat);

    return c;
}

QString HTTPEndpoint::endpointVersion() const
{
    Q_D(const HTTPEndpoint);
    return d->endpointVersion;
}

QUrl HTTPEndpoint::mqttBrokerUrl() const
{
    Q_D(const HTTPEndpoint);
    return d->mqttBroker;
}

bool HTTPEndpoint::isPaired() const
{
    Q_D(const HTTPEndpoint);
    QFileInfo fi(QString("%1/mqtt_broker.crt").arg(pathToAstarteEndpointConfiguration(d->endpointName)));
    return fi.exists();
}

}

#include "moc_httpendpoint.cpp"
