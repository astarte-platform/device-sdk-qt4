/*
 *
 */

#include "transport.h"

#include "httpendpoint.h"
#include "transportcache.h"
#include "mqttclientwrapper.h"
#include "producerabstractinterface.h"

#include "wave.h"
#include "rebound.h"
#include "fluctuation.h"

#include "utils/hemeraoperation.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QStringList>
#include <QtCore/QSocketNotifier>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QVariantMap>

#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <QtCore/QFile>
#include <QtCore/QtConcurrentRun>

#include "utils/utils.h"

#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#define CONNECTION_RETRY_INTERVAL 15000
#define PAIRING_RETRY_INTERVAL (5 * 60 * 1000)

#define METHOD_WRITE "WRITE"
#define METHOD_ERROR "ERROR"

#define CERTIFICATE_RENEWAL_DAYS 8

namespace Astarte
{

Transport::Transport(const QString &configurationPath, const QByteArray &hardwareId, QObject* parent)
    : AsyncInitObject(parent)
    , m_configurationPath(configurationPath)
    , m_hardwareId(hardwareId)
    , m_rebootTimer(new QTimer(this))
    , m_rebootWhenConnectionFails(false)
    , m_rebootDelayMinutes(600)
{
    qRegisterMetaType<MQTTClientWrapper::Status>();
    connect(this, SIGNAL(introspectionChanged()), this, SLOT(publishIntrospection()));
    connect(this, SIGNAL(introspectionChanged()), this, SLOT(setupClientSubscriptions()));
}

Transport::~Transport()
{
}

void Transport::initImpl()
{
    setParts(2);

    Utils::seedRNG();

    // When we are ready to go, we setup MQTT
    connect(this, SIGNAL(ready()), this, SLOT(setupMqtt()));

    // Load from configuration
    if (!QFile::exists(m_configurationPath)) {
        setInitError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())",
                     QString("Configuration file %1 does not exist").arg(m_configurationPath));
        return;
    }

    QSettings settings(m_configurationPath, QSettings::IniFormat);
    settings.beginGroup(QLatin1String("AstarteTransport")); {
        m_persistencyDir = settings.value(QLatin1String("persistencyDir"), QDir::currentPath()).toString();

        QSettings syncSettings(QString("%1/transportStatus.conf").arg(m_persistencyDir), QSettings::IniFormat);
        m_synced = syncSettings.value(QLatin1String("isSynced"), false).toBool();

        TransportCache::setPersistencyDir(m_persistencyDir);
        connect(TransportCache::instance()->init(), SIGNAL(finished(Hemera::Operation*)), this, SLOT(setOnePartIsReady()));

        m_astarteEndpoint = new Astarte::HTTPEndpoint(m_configurationPath, m_persistencyDir, settings.value(QLatin1String("endpoint")).toUrl(),
                                                      m_hardwareId, QSslConfiguration::defaultConfiguration(), this);

        m_rebootWhenConnectionFails = settings.value(QLatin1String("rebootWhenConnectionFails"), false).toBool();
        m_rebootDelayMinutes = settings.value(QLatin1String("rebootDelayMinutes"), 600).toInt();
        //m_rebootTimer->setTimerType(Qt::VeryCoarseTimer);
        int randomizedRebootDelayms = Utils::randomizedInterval(m_rebootDelayMinutes * 60 * 1000, 0.1);
        m_rebootTimer->setInterval(randomizedRebootDelayms);

        if (m_rebootWhenConnectionFails) {
            qDebug() << "Activating the reboot timer with delay " << (randomizedRebootDelayms / (60 * 1000)) << " minutes";
            m_rebootTimer->start();
        }
        connect(m_rebootTimer, SIGNAL(timeout()), this, SLOT(handleRebootTimerTimeout()));

        connect(m_astarteEndpoint->init(), SIGNAL(finished(Hemera::Operation*)), this, SLOT(onEndpointReady(Hemera::Operation*)));
    } settings.endGroup();
}

void Transport::onEndpointReady(Hemera::Operation *op)
{
    if (op->isError()) {
        // TODO
        return;
    }

    if (!m_mqttBroker.isNull()) {
        m_mqttBroker.data()->disconnectFromBroker();
        m_mqttBroker.data()->deleteLater();
    }

    // Are we paired?
    if (!m_astarteEndpoint->isPaired()) {
        startPairing(false);
    } else {
        setOnePartIsReady();
    }
}

void Transport::startPairing(bool forcedPairing) {
    m_isPairingForced = forcedPairing;
    restartPairing();
}

void Transport::restartPairing() {
    connect(m_astarteEndpoint->pair(m_isPairingForced), SIGNAL(finished(Hemera::Operation*)), this, SLOT(onPairingFinished(Hemera::Operation*)));
}

void Transport::onPairingFinished(Hemera::Operation *pOp)
{
    if (pOp->isError()) {
        int retryInterval = Utils::randomizedInterval(PAIRING_RETRY_INTERVAL, 1.0);
        qWarning() << "Pairing failed!!" << pOp->errorMessage() << ", retrying in " << (retryInterval / 1000) << " seconds";
        QTimer::singleShot(retryInterval, this, SLOT(restartPairing()));
        return;
    } else {
        if (m_isPairingForced) {
            setupMqtt();
        } else {
            setOnePartIsReady();
        }
    }
}

void Transport::setupMqtt()
{
    // Good. Let's set up our MQTT broker.
    m_mqttBroker = m_astarteEndpoint->createMqttClientWrapper();

    if (m_mqttBroker.isNull()) {
        qWarning() << "Could not create the MQTT client!!";
        return;
    }

    if (m_mqttBroker.data()->clientCertificateExpiry().isValid() &&
        QDateTime::currentDateTime().daysTo(m_mqttBroker.data()->clientCertificateExpiry()) <= CERTIFICATE_RENEWAL_DAYS) {
        forceNewPairing();
        return;
    }

    // Ok to synchronize this.
    m_mqttBroker.data()->init()->synchronize();
    m_mqttBroker.data()->setKeepAlive(60);
    m_mqttBroker.data()->connectToBroker();

    connect(m_mqttBroker.data(), SIGNAL(statusChanged(Astarte::MQTTClientWrapper::Status)), this, SLOT(onStatusChanged(Astarte::MQTTClientWrapper::Status)));
    connect(m_mqttBroker.data(), SIGNAL(messageReceived(QByteArray,QByteArray)), this, SLOT(onMQTTMessageReceived(QByteArray,QByteArray)));
    connect(m_mqttBroker.data(), SIGNAL(publishConfirmed(int)), this, SLOT(onPublishConfirmed(int)));
    connect(m_mqttBroker.data(), SIGNAL(connackTimeout()), this, SLOT(handleConnackTimeout()));
    connect(m_mqttBroker.data(), SIGNAL(connectionFailed()), this, SLOT(handleConnectionFailed()));
}

void Transport::onMQTTMessageReceived(const QByteArray& topic, const QByteArray& payload)
{
    // Normalize the message
    if (!topic.startsWith(m_mqttBroker.data()->rootClientTopic())) {
        qWarning() << "Received MQTT message on topic" << topic << ", which does not match the device base hierarchy!";
        return;
    }

    QByteArray relativeTopic = topic;
    relativeTopic.remove(0, m_mqttBroker.data()->rootClientTopic().length());

    // Prepare a write wave
    Wave w;
    w.setMethod(METHOD_WRITE);
    w.setTarget(relativeTopic);
    w.setPayload(payload);
    m_waveStorage.insert(w.id(), w);
    qDebug() << "Sending wave" << w.method() << w.target();
    routeWave(w, -1);
}

void Transport::setupClientSubscriptions()
{
    if (m_mqttBroker.isNull()) {
        qWarning() << "Can't publish subscriptions, broker is null";
        return;
    }
    // Setup subscriptions to control interface
    m_mqttBroker.data()->subscribe(m_mqttBroker.data()->rootClientTopic() + "/control/#", MQTTClientWrapper::ExactlyOnceQoS);
    // Setup subscriptions to interfaces where we can receive data
    for (QHash< QByteArray, AstarteInterface >::const_iterator i = introspection().constBegin(); i != introspection().constEnd(); ++i) {
        if (i.value().interfaceQuality() == AstarteInterface::Consumer) {
            // Subscribe to the interface itself
            m_mqttBroker.data()->subscribe(m_mqttBroker.data()->rootClientTopic() + "/" + i.value().interface(), MQTTClientWrapper::ExactlyOnceQoS);
            // Subscribe to the interface properties
            m_mqttBroker.data()->subscribe(m_mqttBroker.data()->rootClientTopic() + "/" + i.value().interface() + "/#", MQTTClientWrapper::ExactlyOnceQoS);
        }
    }
}

void Transport::sendProperties()
{
    for (QHash< QByteArray, QByteArray >::const_iterator i = TransportCache::instance()->allPersistentEntries().constBegin();
         i != TransportCache::instance()->allPersistentEntries().constEnd();
         ++i) {

        // Recreate the cacheMessage
        CacheMessage c;
        c.setTarget(i.key());
        c.setPayload(i.value());
        c.setInterfaceType(AstarteInterface::Properties);

        if (m_mqttBroker.isNull()) {
            handleFailedPublish(c);
            continue;
        }

        int rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + i.key(), i.value(), MQTTClientWrapper::ExactlyOnceQoS);
        if (rc < 0) {
            // If it's < 0, it's an error
            handleFailedPublish(c);
        } else {
            // Otherwise, it's the messageId
            qDebug() << "Inserting in-flight message id " << rc;
            TransportCache::instance()->addInFlightEntry(rc, c);
        }
    }
}

void Transport::resendFailedMessages()
{
    QList<int> ids = TransportCache::instance()->allRetryIds();
    Q_FOREACH (int id, ids) {
        CacheMessage failedMessage = TransportCache::instance()->takeRetryEntry(id);
        // Call cache message function with the failed message
        cacheMessage(failedMessage);
    }
}

void Transport::rebound(const Rebound& r, int fd)
{
    Q_UNUSED(fd);

    Rebound rebound = r;

    // FIXME: We should just trigger errors here.
    return;

    if (!m_waveStorage.contains(r.id())) {
        qWarning() << "Got a rebound with id" << r.id() << "which does not match any routing rules!";
        return;
    }

    Wave w = m_waveStorage.take(r.id());
    QByteArray waveTarget = w.target();

    if (m_commandTree.contains(r.id())) {
        QByteArray commandId = m_commandTree.take(r.id());
        m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + "/response" + waveTarget,
                              commandId + "," + QByteArray::number(static_cast<quint16>(r.response())));
    }

    if (r.response() != OK) {
        // TODO: How do we handle failed requests?
        qWarning() << "Wave failed!" << static_cast<quint16>(r.response());
        return;
    }

    if (!r.payload().isEmpty()) {
        m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + waveTarget, r.payload());
    }
}

void Transport::fluctuation(const Fluctuation &fluctuation)
{
    qDebug() << "Received fluctuation from: " << fluctuation.target() << fluctuation.payload();
}

void Transport::cacheMessage(const CacheMessage &cacheMessage)
{
    qDebug() << "Received cacheMessage from: " << cacheMessage.target() << cacheMessage.payload();
    if (m_mqttBroker.isNull()) {
        handleFailedPublish(cacheMessage);
        return;
    }

    int rc;

    switch (cacheMessage.interfaceType()) {
        case AstarteInterface::Properties: {
            if (TransportCache::instance()->isCached(cacheMessage.target())
                    && TransportCache::instance()->persistentEntry(cacheMessage.target()) == cacheMessage.payload()) {

                qDebug() << cacheMessage.target() << "is not changed, not publishing it again";
                // We consider it delivered, so remove it from the DB
                TransportCache::instance()->removeFromDatabase(cacheMessage);
                return;
            }

            rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + cacheMessage.target(), cacheMessage.payload(), MQTTClientWrapper::ExactlyOnceQoS);
            break;
        }

        case AstarteInterface::DataStream: {
            Reliability reliability = static_cast<Reliability>(cacheMessage.attributes().value("reliability").toInt());
            switch (reliability) {
                case (Guaranteed):
                    rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + cacheMessage.target(), cacheMessage.payload(), MQTTClientWrapper::AtLeastOnceQoS);
                    break;
                case (Unique):
                    rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + cacheMessage.target(), cacheMessage.payload(), MQTTClientWrapper::ExactlyOnceQoS);
                    break;
                default:
                    // Default Unreliable
                    rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + cacheMessage.target(), cacheMessage.payload(), MQTTClientWrapper::AtMostOnceQoS);
                    break;
            }
            break;
        }

        default: {
            rc = -1;
            qDebug() << "Unsupported interfaceType";
            break;
        }
    }

    if (rc < 0) {
        // If it's < 0, it's an error
        handleFailedPublish(cacheMessage);
    } else {
        // Otherwise, it's the messageId
        qDebug() << "Inserting in-flight message id " << rc;
        TransportCache::instance()->addInFlightEntry(rc, cacheMessage);
    }
}

void Transport::forceNewPairing()
{
    // Operation is error, certificate is invalid
    qWarning() << "Forcing new pairing";

    if (!m_mqttBroker.isNull()) {
        m_mqttBroker.data()->disconnectFromBroker();
        m_mqttBroker.data()->deleteLater();
    }
    // Reset the cache
    TransportCache::instance()->resetInFlightEntries();

    startPairing(true);
}

void Transport::onStatusChanged(Astarte::MQTTClientWrapper::Status status)
{
    if (status == MQTTClientWrapper::ConnectedStatus) {
        // We're connected, stop the reboot timer
        qDebug() << "Connected, stopping the reboot timer";
        m_rebootTimer->stop();
        if (!m_mqttBroker.data()->sessionPresent() || !m_synced) {
            // We're desynced
            bigBang();
        }

        // Resend the messages that failed to be published
        resendFailedMessages();
    } else {
        // If we are in every other state, we start the reboot timer (if needed)
        if (m_rebootWhenConnectionFails && !m_rebootTimer->isActive()) {
            qDebug() << "Not connected state, restarting the reboot timer";
            m_rebootTimer->start();
        }
    }
}

void Transport::handleConnectionFailed()
{
    int retryInterval = Utils::randomizedInterval(CONNECTION_RETRY_INTERVAL, 1.0);
    qDebug() << "Connection failed, trying to reconnect to the broker in " << (retryInterval / 1000) << " seconds";
    QTimer::singleShot(retryInterval, m_mqttBroker.data(), SLOT(connectToBroker()));
}

void Transport::handleConnackTimeout()
{
    qWarning() << "CONNACK timeout, verifying certificate";

    if(!m_astarteEndpoint->verifyCertificate()->synchronize()) {
        qWarning() << "Certificate verification failed";
        forceNewPairing();
    }
}

void Transport::handleRebootTimerTimeout()
{
    qWarning() << "Connection with Astarte was lost for too long!";
}

void Transport::handleFailedPublish(const CacheMessage &cacheMessage)
{
    qWarning() << "Can't publish for target" << cacheMessage.target();
    if (cacheMessage.attributes().value("retention").toInt() == static_cast<int>(Discard)) {
        // Prepare an error wave
        Wave w;
        w.setMethod(METHOD_ERROR);
        w.setTarget(cacheMessage.target());
        w.setPayload(cacheMessage.payload());
        qDebug() << "Sending error wave for Discard target " << w.target();
        routeWave(w, -1);
    } else {
        int id = TransportCache::instance()->addRetryEntry(cacheMessage);
        Q_UNUSED(id);
    }
}

void Transport::bigBang()
{
    qWarning() << "Received bigBang";

    QSettings syncSettings(QString("%1/transportStatus.conf").arg(m_persistencyDir), QSettings::IniFormat);
    if (m_synced) {
        m_synced = false;
        syncSettings.setValue(QLatin1String("isSynced"), false);
    }

    if (m_mqttBroker.isNull()) {
        qDebug() << "Can't send emptyCache request, broker is null";
        return;
    }

    // We need to setup again the subscriptions, unless we have a persistent session on the other end.
    setupClientSubscriptions();
    // And publish the introspection.
    publishIntrospection();

    int rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + "/control/emptyCache", "1", MQTTClientWrapper::ExactlyOnceQoS);
    if (rc < 0) {
        // We leave m_synced to false and we retry when we're back online
        qWarning() << "Can't send emptyCache request, error " << rc;
        return;
    }

    QByteArray payload;
    Q_FOREACH (const QByteArray &path, TransportCache::instance()->allPersistentEntries().keys()) {
        // Remove leading slash
        payload.append(path.mid(1));
        payload.append(';');
    }
    // Remove trailing semicolon
    payload.chop(1);

    qDebug() << "Producer property paths are: " << payload;

    rc = m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic() + "/control/producer/properties", qCompress(payload), MQTTClientWrapper::ExactlyOnceQoS);
    if (rc < 0) {
        // We leave m_synced to false and we retry when we're back online
        qWarning() << "Can't send producer properties list, error " << rc;
        return;
    }

    // Send the cached properties
    sendProperties();

    // If we are here, the messages are in the hands of mosquitto
    m_synced = true;
    syncSettings.setValue(QLatin1String("isSynced"), true);
}

void Transport::onPublishConfirmed(int messageId)
{
    qDebug() << "Message with id" << messageId << ": publish confirmed";
    CacheMessage cacheMessage = TransportCache::instance()->takeInFlightEntry(messageId);

    if (cacheMessage.interfaceType() == AstarteInterface::Properties) {
        if (cacheMessage.payload().isEmpty()) {
            TransportCache::instance()->removePersistentEntry(cacheMessage.target());
        } else {
            TransportCache::instance()->insertOrUpdatePersistentEntry(cacheMessage.target(), cacheMessage.payload());
        }
    }
}

void Transport::publishIntrospection()
{
    if (m_mqttBroker.isNull()) {
        return;
    }

    qDebug() << "Publishing introspection!";

    // Create a string representation
    QByteArray payload;
    for (QHash< QByteArray, AstarteInterface >::const_iterator i = introspection().constBegin(); i != introspection().constEnd(); ++i) {
        payload.append(i.key());
        payload.append(':');
        payload.append(QByteArray::number(i.value().versionMajor()));
        payload.append(':');
        payload.append(QByteArray::number(i.value().versionMinor()));
        payload.append(';');
    }

    // Remove last ;
    payload.chop(1);

    qDebug() << "Introspection is " << payload;

    m_mqttBroker.data()->publish(m_mqttBroker.data()->rootClientTopic(), payload, MQTTClientWrapper::ExactlyOnceQoS);
}

QHash< QByteArray, AstarteInterface> Transport::introspection() const
{
    // TODO
    return m_introspection;
}

void Transport::setIntrospection(const QHash< QByteArray, AstarteInterface> &introspection)
{
    if (m_introspection != introspection) {
        m_introspection = introspection;
        emit introspectionChanged();
    }
}

void Transport::routeWave(const Wave &wave, int fd)
{
    Q_UNUSED(fd)

    QByteArray waveTarget = wave.target();
    int targetIndex = waveTarget.indexOf('/', 1);
    QByteArray interface = waveTarget.mid(1, targetIndex - 1);
    QByteArray relativeTarget = waveTarget.right(waveTarget.length() - targetIndex);

    if (interface == "control") {
        qDebug() << "Received control wave, not implemented in SDK";
        return;
    }

    Wave w;
    w.setTarget(relativeTarget);
    w.setPayload(wave.payload());

    Q_EMIT waveReceived(interface, w);
}

}
