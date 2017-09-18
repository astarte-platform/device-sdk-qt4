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
