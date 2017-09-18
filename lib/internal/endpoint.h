#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "utils/hemeraasyncinitobject.h"

#include "crypto.h"

#include <QtCore/QUrl>

class QNetworkReply;

namespace Astarte {

class MQTTClientWrapper;

class EndpointPrivate;
class Endpoint : public Hemera::AsyncInitObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Endpoint)
    Q_DECLARE_PRIVATE_D(d_h_ptr, Endpoint)

public:
    virtual ~Endpoint();

    QUrl endpoint() const;
    virtual QString endpointVersion() const;
    virtual QUrl mqttBrokerUrl() const = 0;

    virtual MQTTClientWrapper *createMqttClientWrapper() = 0;

    virtual bool isPaired() const = 0;

    virtual Hemera::Operation *pair(bool force = true) = 0;

    virtual Hemera::Operation *verifyCertificate() = 0;

    virtual QNetworkReply *sendRequest(const QString &relativeEndpoint, const QByteArray &payload,
                                       Crypto::AuthenticationDomain authenticationDomain = Crypto::DeviceAuthenticationDomain) = 0;

protected:
    explicit Endpoint(EndpointPrivate &dd, const QUrl &endpoint, QObject *parent = 0);

    EndpointPrivate * const d_h_ptr;
};
}

#endif // ENDPOINT_H
