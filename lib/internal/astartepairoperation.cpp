/*
 * Copyright (C) 2017-2018 Ispirata Srl
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

#include "astartepairoperation.h"

#include "httpendpoint.h"
#include "httpendpoint_p.h"

#include <QtCore/QFile>
#include <QtCore/QSettings>

#include <QtNetwork/QNetworkReply>

#include "rapidjson/document.h"


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
        performPairing();
    }
}

void PairOperation::onGenerationFinished(Hemera::Operation *op)
{
    if (op->isError()) {
        // That's ugly.
        setFinishedWithError(op->errorName(), op->errorMessage());
        return;
    }

    performPairing();
}

void PairOperation::performPairing()
{
    QFile csr(Crypto::instance()->pathToCertificateRequest());
    if (!csr.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open CSR for reading! Aborting.";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())", QString("Could not open CSR for reading! Aborting."));
    }

    QByteArray csrBytes = csr.readAll();
    QString jsonDocument = QString("{\"data\":{\"csr\":\"%1\"}}").arg(QString::fromLatin1(csrBytes));

    QNetworkReply *r = m_endpoint->sendRequest(QString("/protocols/astarte_mqtt_v1/credentials"),
                                               jsonDocument.toUtf8(), Crypto::DeviceAuthenticationDomain);

    qDebug() << "I'm sending: " << jsonDocument;

    connect(r, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
}

void PairOperation::onNetworkReplyFinished()
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
    if (!doc.IsObject() || !doc["data"].IsObject()) {
        qWarning() << "Parsing pairing result error!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", QString("Parsing pairing result error!"));
        return;
    }

    if (!doc["data"].HasMember("client_crt")) {
        qWarning() << "Missing certificate in the pairing routine!";
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::badRequest())", QString("Missing certificate in the pairing routine!"));
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
        generatedCertificate.write(doc["data"]["client_crt"].GetString());
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

}
