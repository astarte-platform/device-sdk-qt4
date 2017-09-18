/*
 *
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "mqttclientwrapper.h"

#include <QtCore/QObject>

#include <QtCore/QWeakPointer>
#include <QtCore/QSet>

#include "astarteinterface.h"

#include "utils/hemeraasyncinitobject.h"

class QTimer;

namespace Astarte {
class Endpoint;
}

class Fluctuation;
class Rebound;
class Wave;

namespace Astarte {
class MQTTClientWrapper;
class CacheMessage;
class Interface;

class Transport : public Hemera::AsyncInitObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Transport)

public:
    Transport(const QString &configurationPath, const QByteArray &hardwareId, QObject *parent = 0);
    virtual ~Transport();

    virtual void rebound(const Rebound& rebound, int fd = -1);
    virtual void fluctuation(const Fluctuation& fluctuation);
    virtual void cacheMessage(const CacheMessage& cacheMessage);
    virtual void bigBang();

    QHash< QByteArray, AstarteInterface > introspection() const;
    void setIntrospection(const QHash< QByteArray, AstarteInterface > &introspection);

Q_SIGNALS:
    void introspectionChanged();
    void waveReceived(const QByteArray &interface, const Wave &wave);

protected Q_SLOTS:
    virtual void initImpl();

    void routeWave(const Wave &wave, int fd);

private Q_SLOTS:
    void restartPairing();
    void startPairing(bool forcedPairing);
    void setupMqtt();
    void setupClientSubscriptions();
    void sendProperties();
    void resendFailedMessages();
    void publishIntrospection();
    void onStatusChanged(Astarte::MQTTClientWrapper::Status status);
    void onMQTTMessageReceived(const QByteArray &topic, const QByteArray &payload);
    void onPublishConfirmed(int messageId);
    void handleFailedPublish(const CacheMessage &cacheMessage);
    void handleConnectionFailed();
    void handleConnackTimeout();
    void handleRebootTimerTimeout();
    void forceNewPairing();

    void onPairingFinished(Hemera::Operation *pOp);
    void onEndpointReady(Hemera::Operation *op);

private:
    Astarte::Endpoint *m_astarteEndpoint;
    QWeakPointer<MQTTClientWrapper> m_mqttBroker;
    QHash< quint64, Wave > m_waveStorage;
    QHash< quint64, QByteArray > m_commandTree;
    QHash< QByteArray, AstarteInterface > m_introspection;
    QString m_configurationPath;
    QByteArray m_hardwareId;
    QString m_persistencyDir;
    QTimer *m_rebootTimer;
    bool m_synced;
    bool m_rebootWhenConnectionFails;
    int m_rebootDelayMinutes;
    bool m_isPairingForced;
};
}

#endif // TRANSPORT_H
