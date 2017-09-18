#ifndef MQTTCLIENTWRAPPER_H
#define MQTTCLIENTWRAPPER_H

#include "utils/hemeraasyncinitobject.h"

#include "utils/hemeraoperation.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>

namespace Hemera {
class Operation;
}

namespace Astarte {

class MQTTClientWrapperPrivate;
class MQTTClientWrapper : public Hemera::AsyncInitObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MQTTClientWrapper)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    enum Status {
        UnknownStatus = 0,
        DisconnectedStatus = 1,
        ConnectedStatus = 2,
        ConnectingStatus = 3,
        DisconnectingStatus = 4,
        ReconnectingStatus = 5
    };
    Q_ENUMS(Status);
    enum MQTTQoS {
        AtMostOnceQoS = 0,
        AtLeastOnceQoS = 1,
        ExactlyOnceQoS = 2,
        DefaultQoS = 99
    };
    Q_ENUMS(MQTTQoS);

    explicit MQTTClientWrapper(const QUrl &host, const QByteArray &clientId, QObject *parent = 0);
    virtual ~MQTTClientWrapper();

    Status status() const;
    QByteArray hardwareId() const;
    QByteArray rootClientTopic() const;
    QDateTime clientCertificateExpiry() const;
    bool sessionPresent() const;

    void setMutualSSLAuthentication(const QString &pathToCA, const QString &pathToPKey, const QString &pathToCertificate);
    void setPublishQoS(MQTTQoS qos);
    void setSubscribeQoS(MQTTQoS qos);
    void setIgnoreSslErrors(bool ignoreSslErrors);
    /// Note: this will only work if set before initializing the Client.
    void setCleanSession(bool cleanSession = true);
    void setKeepAlive(quint64 seconds = 300);
    void setLastWill(const QByteArray &topic, const QByteArray &message, MQTTQoS qos, bool retained = false);

    int publish(const QByteArray &topic, const QByteArray &payload, MQTTQoS qos = DefaultQoS, bool retained = false);
    void subscribe(const QByteArray &topic, MQTTQoS qos = DefaultQoS);

public Q_SLOTS:
    bool connectToBroker();
    bool disconnectFromBroker();

protected Q_SLOTS:
    virtual void initImpl();

Q_SIGNALS:
    void messageReceived(const QByteArray &topic, const QByteArray &payload);
    void statusChanged(Astarte::MQTTClientWrapper::Status status);
    void connectionLost(const QString &cause);
    void publishConfirmed(int mid);
    void connectionFailed();
    void connectionStarted();
    void connackReceived();
    void connackTimeout();

private:
    class Private;
    Private * const d;

    friend class HyperdriveMosquittoClient;
};
}

Q_DECLARE_METATYPE(Astarte::MQTTClientWrapper::Status)

#endif // MQTTCLIENTWRAPPER_H
