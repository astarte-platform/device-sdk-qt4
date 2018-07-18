/*
 * Copyright (C) 2018 Ispirata Srl
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

#include "defaultcredentialssecretprovider.h"

#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include "utils/utils.h"

#include "rapidjson/document.h"

#define RETRY_INTERVAL 15000


namespace Astarte {

DefaultCredentialsSecretProvider::DefaultCredentialsSecretProvider(HTTPEndpoint *parent)
    : CredentialsSecretProvider(parent)
{
}

DefaultCredentialsSecretProvider::~DefaultCredentialsSecretProvider()
{
}

void DefaultCredentialsSecretProvider::ensureCredentialsSecret()
{
    QSettings settings(QString("%1/endpoint_crypto.conf").arg(m_endpointConfigurationPath), QSettings::IniFormat);
    if (settings.contains(QString("credentialsSecret"))) {
        m_credentialsSecret = settings.value(QString("credentialsSecret")).toString().toLatin1();
        emit credentialsSecretReady(m_credentialsSecret);
    } else {
        sendRegistrationRequest();
    }
}

QByteArray DefaultCredentialsSecretProvider::credentialsSecret() const
{
    return m_credentialsSecret;
}

void DefaultCredentialsSecretProvider::setAgentKey(const QByteArray &agentKey)
{
    m_agentKey = agentKey;
}

void DefaultCredentialsSecretProvider::setEndpointConfigurationPath(const QString &endpointConfigurationPath)
{
    m_endpointConfigurationPath = endpointConfigurationPath;
}

void DefaultCredentialsSecretProvider::setEndpointUrl(const QUrl &endpointUrl)
{
    m_endpointUrl = endpointUrl;
}

void DefaultCredentialsSecretProvider::setHardwareId(const QByteArray &hardwareId)
{
    m_hardwareId = hardwareId;
}

void DefaultCredentialsSecretProvider::setNAM(QNetworkAccessManager *nam)
{
    m_nam = nam;
}

void DefaultCredentialsSecretProvider::setSslConfiguration(const QSslConfiguration &configuration)
{
    m_sslConfiguration = configuration;
}

void DefaultCredentialsSecretProvider::sendRegistrationRequest()
{
    if (!m_nam) {
        qWarning() << "NAM is null, can't send registration request";
        retryRegistrationLater();
        return;
    }

    qWarning() << "Registering the device";
    QString jsonDocument = QString("{\"data\":{\"hw_id\":\"%1\"}}").arg(QString(m_hardwareId));

    QNetworkRequest req;
    req.setSslConfiguration(m_sslConfiguration);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/json"));
    req.setRawHeader("Authorization", "Bearer " + m_agentKey);
    QUrl reqUrl = m_endpointUrl;
    reqUrl.setPath(m_endpointUrl.path() + QString("/agent/devices"));
    req.setUrl(reqUrl);

    qDebug() << "Sending registration request.";
    qDebug() << "URL:" << req.url() << "Payload:" << jsonDocument;

    QNetworkReply *r = m_nam->post(req, jsonDocument.toUtf8());
    connect(r, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
}

void DefaultCredentialsSecretProvider::onNetworkReplyFinished()
{
    QNetworkReply *r = qobject_cast<QNetworkReply*>(sender());

    if (r->error() != QNetworkReply::NoError) {
        qWarning() << "Registration error! Error: " << r->error();
        retryRegistrationLater();
        r->deleteLater();
        return;
    }

    QByteArray replyBytes = r->readAll();
    rapidjson::Document doc;
    doc.Parse(replyBytes);
    r->deleteLater();
    qDebug() << "Device registered";
    if (!doc.IsObject()) {
        qWarning() << "Registration result is not a valid JSON document: " << replyBytes;
        retryRegistrationLater();
        return;
    }

    QString credentialsSecretString = doc["data"]["credentials_secret"].GetString();
    if (credentialsSecretString.isEmpty()) {
        qWarning() << "Missing credentials_secret in the response: ";
        retryRegistrationLater();
        return;
    }

    m_credentialsSecret = credentialsSecretString.toLatin1();

    // Ok, we need to write the files now.
    {
        QSettings settings(QString("%1/endpoint_crypto.conf").arg(m_endpointConfigurationPath),
                            QSettings::IniFormat);
        settings.setValue(QString("credentialsSecret"), credentialsSecretString);
    }

    emit credentialsSecretReady(m_credentialsSecret);
}

void DefaultCredentialsSecretProvider::retryRegistrationLater()
{
    int retryInterval = Utils::randomizedInterval(RETRY_INTERVAL, 1.0);
    qWarning() << "Retrying in" << (retryInterval / 1000) << "seconds";
    QTimer::singleShot(retryInterval, this, SLOT(sendRegistrationRequest()));
}

}
