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

#ifndef HTTPENDPOINT_H
#define HTTPENDPOINT_H

#include "endpoint.h"

#include "crypto.h"

#include <QtCore/QUrl>

#include <QtNetwork/QSslConfiguration>

class QNetworkReply;

namespace Astarte {

class HTTPEndpointPrivate;
class HTTPEndpoint : public Endpoint
{
    Q_OBJECT
    Q_DISABLE_COPY(HTTPEndpoint)
    Q_DECLARE_PRIVATE_D(d_h_ptr, HTTPEndpoint)

    Q_PRIVATE_SLOT(d_func(), void connectToEndpoint())
    Q_PRIVATE_SLOT(d_func(), void onConnectionEstablished())
    Q_PRIVATE_SLOT(d_func(), void processCryptoStatus())
    Q_PRIVATE_SLOT(d_func(), void onCredentialsSecretReady())

    friend class PairOperation;

public:
    explicit HTTPEndpoint(const QString &configurationFile, const QString &persistencyDir, const QUrl &endpoint, const QByteArray &hardwareId,
                          const QSslConfiguration &sslConfiguration = QSslConfiguration(), QObject *parent = 0);
    virtual ~HTTPEndpoint();

    QUrl endpoint() const;
    virtual QString endpointVersion() const;
    virtual QUrl mqttBrokerUrl() const;

    virtual MQTTClientWrapper *createMqttClientWrapper();

    virtual bool isPaired() const;

    virtual Hemera::Operation *pair(bool force = true);

    virtual Hemera::Operation *verifyCertificate();

    virtual QNetworkReply *sendRequest(const QString &relativeEndpoint, const QByteArray &payload,
                                       Crypto::AuthenticationDomain authenticationDomain = Crypto::DeviceAuthenticationDomain);

protected Q_SLOTS:
    virtual void initImpl();
    void ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QString endpointConfigurationBasePath() const;
    QString pathToAstarteEndpointConfiguration(const QString &endpointName) const;
};

}


#endif // HTTPENDPOINT_H
