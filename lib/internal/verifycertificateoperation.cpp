#include "verifycertificateoperation.h"

#include "httpendpoint.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>

#include <QtNetwork/QNetworkReply>

#include "rapidjson/document.h"
#include "utils/utils.h"

#define RETRY_INTERVAL 15000

namespace Astarte {

VerifyCertificateOperation::VerifyCertificateOperation(QFile &certFile, HTTPEndpoint *parent)
    : Hemera::Operation(parent)
    , m_endpoint(parent)
{
    if (certFile.open(QIODevice::ReadOnly)) {
        m_certificate = certFile.readAll();
    } else {
        qWarning() << "Cannot open " << certFile.fileName() << " reason: " << certFile.error();
    }
}

VerifyCertificateOperation::VerifyCertificateOperation(const QByteArray &certificate, HTTPEndpoint *parent)
    : Hemera::Operation(parent)
    , m_certificate(certificate)
    , m_endpoint(parent)
{
}

VerifyCertificateOperation::VerifyCertificateOperation(const QSslCertificate &certificate, HTTPEndpoint *parent)
    : Hemera::Operation(parent)
    , m_certificate(certificate.toPem())
    , m_endpoint(parent)
{
}

VerifyCertificateOperation::~VerifyCertificateOperation()
{
}

void VerifyCertificateOperation::startImpl()
{
    if (m_certificate.isEmpty()) {
        qWarning() << "Empty certificate";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())", QLatin1String("Invalid certificate"));
        return;
    }

    verify();
}

void VerifyCertificateOperation::verify()
{
    QNetworkReply *r = m_endpoint->sendRequest(QLatin1String("/verifyCertificate"), m_certificate, Crypto::DeviceAuthenticationDomain);

    connect(r, SIGNAL(finished()), this, SLOT(onReplyFinished()));
}

void VerifyCertificateOperation::onReplyFinished()
{
    QNetworkReply *r = qobject_cast<QNetworkReply*>(sender());
    if (r->error() != QNetworkReply::NoError) {
        int retryInterval = Utils::randomizedInterval(RETRY_INTERVAL, 1.0);
        qWarning() << "Error while verifying certificate! Retrying in " << (retryInterval / 1000) << " seconds. error: " << r->error();

        // We never give up. If we couldn't connect, we reschedule this in 15 seconds.
        QTimer::singleShot(retryInterval, this, SLOT(verify()));
        r->deleteLater();
        return;
    } else {
        rapidjson::Document doc;
        doc.Parse(r->readAll().constData());
        r->deleteLater();
        if (!doc.IsObject()) {
            int retryInterval = Utils::randomizedInterval(RETRY_INTERVAL, 1.0);
            qWarning() << "Parsing error, resending request in " << (retryInterval / 1000) << " seconds.";
            QTimer::singleShot(retryInterval, this, SLOT(verify()));
            return;
        }

        // TODO: handle timestamp field
        if (doc["valid"].GetBool()) {
            qDebug() << "Valid certificate";
            setFinished();
            return;
        } else {
            qWarning() << "Invalid certificate";
            setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())",
                                 QString("Invalid certificate (%1): %2")
                                 .arg(doc["cause"].GetString(), doc["details"].GetString()));
            return;
        }
    }
}

}
