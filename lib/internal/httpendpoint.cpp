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

#include "httpendpoint.h"
#include "httpendpoint_p.h"

#include "astartepairoperation.h"
#include "crypto.h"
#include "credentialssecretprovider.h"
#include "defaultcredentialssecretprovider.h"
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

void HTTPEndpointPrivate::connectToEndpoint()
{
    QUrl infoEndpoint = endpoint;
    infoEndpoint.setPath(QString("%1/devices/%2").arg(endpoint.path()).arg(QString::fromLatin1(hardwareId)));
    QNetworkRequest req(infoEndpoint);
    req.setSslConfiguration(sslConfiguration);
    req.setRawHeader("Authorization", "Bearer " + credentialsSecretProvider->credentialsSecret());
    req.setRawHeader("X-Astarte-Transport-Provider", "Astarte Device SDK Qt4");
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
        retryConnectToEndpointLater();
        reply->deleteLater();
        return;
    }

    rapidjson::Document doc;
    doc.Parse(reply->readAll());
    reply->deleteLater();
    if (!doc.IsObject()) {
        retryConnectToEndpointLater();
        return;
    }

    qDebug() << "Connected! ";

    endpointVersion = doc["data"]["version"].GetString();

    if (!doc["data"]["protocols"].HasMember("astarte_mqtt_v1") || !doc["data"]["protocols"]["astarte_mqtt_v1"].HasMember("broker_url")) {
        qWarning() << "No broker_url or astarte_mqtt_v1 protocol available from info endpoint";
        retryConnectToEndpointLater();
        return;
    }

    QString status = doc["data"]["status"].GetString();

    qDebug() << "Device status is " << status;

    // Get configuration
    QSettings settings(QString("%1/mqtt_broker.conf").arg(q->pathToAstarteEndpointConfiguration(endpointName)),
                       QSettings::IniFormat);
    mqttBroker = QUrl::fromUserInput(doc["data"]["protocols"]["astarte_mqtt_v1"]["broker_url"].GetString());
    qDebug() << "Broker url is " << mqttBroker;

    if (Crypto::instance()->isReady() || Crypto::instance()->hasInitError()) {
        processCryptoStatus();
    } else {
        Crypto::instance()->setHardwareId(hardwareId);
        QObject::connect(Crypto::instance()->init(), SIGNAL(finished(Hemera::Operation*)), q, SLOT(processCryptoStatus()));
    }
}

void HTTPEndpointPrivate::retryConnectToEndpointLater()
{
    Q_Q(HTTPEndpoint);
    int retryInterval = Utils::randomizedInterval(RETRY_INTERVAL, 1.0);
    qWarning() << "Error while connecting to info endpoint, retrying in " << (retryInterval / 1000) << " seconds";
    QTimer::singleShot(retryInterval, q, SLOT(connectToEndpoint()));
}

void HTTPEndpointPrivate::ensureCredentialsSecret()
{
    Q_Q(HTTPEndpoint);
    DefaultCredentialsSecretProvider *provider = new DefaultCredentialsSecretProvider(q);
    provider->setAgentKey(agentKey);
    provider->setEndpointConfigurationPath(q->pathToAstarteEndpointConfiguration(endpointName));
    provider->setEndpointUrl(endpoint);
    provider->setHardwareId(hardwareId);
    provider->setNAM(nam);
    provider->setSslConfiguration(sslConfiguration);
    credentialsSecretProvider = provider;

    QObject::connect(credentialsSecretProvider, SIGNAL(credentialsSecretReady(QByteArray)), q, SLOT(onCredentialsSecretReady()));

    credentialsSecretProvider->ensureCredentialsSecret();
}

void HTTPEndpointPrivate::onCredentialsSecretReady()
{
    qDebug() << "Credentials secret is: " << credentialsSecretProvider->credentialsSecret();
    connectToEndpoint();
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

    d->ensureCredentialsSecret();

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
    QNetworkRequest req;
    if (authenticationDomain == Crypto::DeviceAuthenticationDomain) {
        // Build the endpoint
        QUrl target = d->endpoint;
        target.setPath(QString("%1/devices/%2%3").arg(d->endpoint.path()).arg(QString::fromLatin1(d->hardwareId)).arg(relativeEndpoint));
        req.setUrl(target);

        req.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/json"));
        req.setSslConfiguration(d->sslConfiguration);

        qDebug() << "Request is: " << relativeEndpoint << payload << authenticationDomain;

        req.setRawHeader("Authorization", "Bearer " + d->credentialsSecretProvider->credentialsSecret());
        req.setRawHeader("X-Astarte-Transport-Provider", "Astarte Device SDK Qt4");
        req.setRawHeader("X-Astarte-Transport-Version", QString("%1.%2.%3")
                                                     .arg(Utils::majorVersion())
                                                     .arg(Utils::minorVersion())
                                                     .arg(Utils::releaseVersion())
                                                     .toLatin1());
    } else {
        qWarning() << "Only DeviceAuthenticationDomain can be used in astartehttpendpoint";
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
