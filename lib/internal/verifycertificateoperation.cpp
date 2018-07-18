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
    QString jsonDocument = QString("{\"data\":{\"client_crt\":\"%1\"}}").arg(QString::fromLatin1(m_certificate));

    QNetworkReply *r = m_endpoint->sendRequest(QString("/protocols/astarte_mqtt_v1/credentials/verify"),
                                               jsonDocument.toUtf8(), Crypto::DeviceAuthenticationDomain);

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

        QString timestamp = doc["data"]["timestamp"].GetString();
        if (doc["data"]["valid"].GetBool()) {
            qDebug() << "Valid certificate, until" << doc["data"]["until"].GetString();
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
