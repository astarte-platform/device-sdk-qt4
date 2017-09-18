#include "mqttclientwrapper.h"


#include "utils/utils.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtCore/QMetaMethod>

#include <QtNetwork/QSslCertificate>

#include "utils/hemeracommonoperations.h"

// We use the as variant
#include <mosquittopp.h>

#define CONNACK_TIMEOUT (2 * 60 * 1000)

namespace Astarte {

class HyperdriveMosquittoClient;

class MQTTClientWrapper::Private
{
public:
    Private(MQTTClientWrapper* q) : q_ptr(q)
                               , mosquitto(0)
                               , status(MQTTClientWrapper::DisconnectedStatus)
                               , lwtRetained(false)
                               , keepAlive(5 * 60)
                               , cleanSession(false)
                               , sessionPresent(false)
                               , publishQoS(1)
                               , subscribeQoS(1) {}

    Q_DECLARE_PUBLIC(MQTTClientWrapper)
    MQTTClientWrapper * const q_ptr;

    HyperdriveMosquittoClient *mosquitto;

    MQTTClientWrapper::Status status;
    QByteArray hardwareId;
    QByteArray customerId;

    QByteArray lwtTopic;
    QByteArray lwtMessage;
    MQTTClientWrapper::MQTTQoS lwtQos;
    bool lwtRetained;

    quint64 keepAlive;
    bool cleanSession;
    bool sessionPresent;
    bool ignoreSslErrors;
    int publishQoS;
    int subscribeQoS;
    QUrl serverUrl;
    QDateTime clientCertificateExpiry;
    QTimer *connackTimer;

    // SSL
    QString pathToCA;
    QString pathToPKey;
    QString pathToCertificate;

    void setStatus(MQTTClientWrapper::Status s);

    // MQTT CALLBACKS
    void on_connect(int rc);
    void on_disconnect(int rc);
    void on_publish(int mid);
    void on_message(const struct mosquitto_message *message);
    void on_subscribe(int mid, int qos_count, const int *granted_qos);
    void on_unsubscribe(int mid);
    void on_log(int level, const char *str);
    void on_error();
};

class HyperdriveMosquittoClient : public mosqpp::mosquittopp
{
public:
    explicit HyperdriveMosquittoClient(MQTTClientWrapper::Private *d, const char *id, bool clean_session)
        : mosquittopp(id, clean_session)
        , d(d) {}
    virtual ~HyperdriveMosquittoClient() {}

    // MQTT CALLBACKS. Just redirect to our private class
    inline virtual void on_connect(int rc) { d->on_connect(rc); }
    inline virtual void on_disconnect(int rc) { d->on_disconnect(rc); }
    inline virtual void on_publish(int mid) { d->on_publish(mid); }
    inline virtual void on_message(const struct mosquitto_message *message) { d->on_message(message); }
    inline virtual void on_subscribe(int mid, int qos_count, const int *granted_qos) { d->on_subscribe(mid, qos_count, granted_qos); }
    inline virtual void on_unsubscribe(int mid) { d->on_unsubscribe(mid); }
    inline virtual void on_log(int level, const char *str) { d->on_log(level, str); }
    inline virtual void on_error() { d->on_error(); }

private:
    MQTTClientWrapper::Private *d;
};

void MQTTClientWrapper::Private::setStatus(MQTTClientWrapper::Status s)
{
    if (status != s) {
        status = s;
        Q_Q(MQTTClientWrapper);
        Q_EMIT q->statusChanged(status);
    }
}

void MQTTClientWrapper::Private::on_connect(int rc)
{
    qDebug() << "Connected to broker returned!";

    Q_Q(MQTTClientWrapper);

    // We received a CONNACK, stop the timer
    Q_EMIT q->connackReceived();

    if (rc == MOSQ_ERR_SUCCESS) {
        // TODO: check is session present with patched mosquitto
        sessionPresent = false;
        qDebug() << "Connected to broker, session present: " << sessionPresent;
        setStatus(MQTTClientWrapper::ConnectedStatus);
    } else {
        qDebug() << "Could not connected to broker!" << rc;
    }
}

void MQTTClientWrapper::Private::on_disconnect(int rc)
{
    setStatus(MQTTClientWrapper::DisconnectedStatus);

    if (rc == 0) {
        // Client requested disconnect.
        mosquitto->loop_stop();
    } else {
        // Unexpected disconnect, Mosquitto will reconnect
        qDebug() << "Unexpected disconnection from broker!" << rc;
        setStatus(MQTTClientWrapper::ReconnectingStatus);

        Q_Q(MQTTClientWrapper);
        Q_EMIT q->connectionStarted();
    }
}

// void MQTTClientWrapper::Private::onDisconnectionFailed(void* context, MQTTAsync_failureData* response)
// {
//     DisconnectFromBrokerOperation *op = (DisconnectFromBrokerOperation*)context;
//
//     op->m_client->d_func()->setStatus(MQTTClientWrapper::UnknownStatus);
//
//     if (response) {
//         op->setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest()),
//                                  QStringLiteral("Disconnection failed! %1: %2.").arg(response->code).arg(QLatin1String(response->message)));
//
//         // Free the response
//         MQTTAsync_free(response);
//     } else {
//         op->setFinishedWithError(Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest()),
//                                  QStringLiteral("Disconnection failed! No response given."));
//     }
// }

// void MQTTClientWrapper::Private::onConnectionLost(void* context, char* cause)
// {
//     MQTTClientWrapper *instance = (MQTTClientWrapper*)context;
//
//     if (cause) {
//         qDebug() << "Connection lost!" << cause;
//         MQTTAsync_free(cause);
//     } else {
//         qDebug() << "Connection lost! No cause given.";
//     }
//
//     instance->d_func()->setStatus(MQTTClientWrapper::DisconnectedStatus);
//     Q_EMIT instance->connectionLost(QString::fromLatin1(cause));
//
//     if (instance->d_func()->reconnectAutomatically) {
//         instance->metaObject()->invokeMethod(instance, "connectToBroker", Qt::QueuedConnection);
//     }
// }

void MQTTClientWrapper::Private::on_message(const struct mosquitto_message *message)
{
    Q_Q(MQTTClientWrapper);

    // We need to copy the bytes. It's impossible to control the ownership otherwise.
    // Do not use ::fromRawData until you find out how to do 0-copy without leaking!!
    QByteArray payload((char*)message->payload, message->payloadlen);
    QByteArray topic(message->topic);

    Q_EMIT q->messageReceived(topic, payload);

    // Free message and topic??

}

void MQTTClientWrapper::Private::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
    Q_UNUSED(mid);
    Q_UNUSED(qos_count);
    Q_UNUSED(granted_qos);
}

void MQTTClientWrapper::Private::on_unsubscribe(int mid)
{
    Q_UNUSED(mid);
}

void MQTTClientWrapper::Private::on_log(int level, const char *str)
{
    qWarning() << "MOSQUITTO LOG!" << level << str;
}

void MQTTClientWrapper::Private::on_error()
{
    qWarning() << "MOSQUITTO ERROR!";
}

void MQTTClientWrapper::Private::on_publish(int mid)
{
    Q_Q(MQTTClientWrapper);

    Q_EMIT q->publishConfirmed(mid);
}


MQTTClientWrapper::MQTTClientWrapper(const QUrl &host, const QByteArray &clientId, QObject *parent)
    : AsyncInitObject(parent)
    , d(new MQTTClientWrapper::Private(this))
{
    d->hardwareId = clientId;
    d->serverUrl = host;
    d->connackTimer = new QTimer(this);
    d->connackTimer->setInterval(Utils::randomizedInterval(CONNACK_TIMEOUT, 1.0));
    d->connackTimer->setSingleShot(true);
}

MQTTClientWrapper::~MQTTClientWrapper()
{
    if (Q_LIKELY(d->mosquitto)) {
        qWarning() << "Stopping mosquitto!";
        d->mosquitto->disconnect();
        d->mosquitto->loop_stop();

        delete d->mosquitto;
        mosqpp::lib_cleanup();
    }
}

MQTTClientWrapper::Status MQTTClientWrapper::status() const
{
    return d->status;
}

QDateTime MQTTClientWrapper::clientCertificateExpiry() const
{

    return d->clientCertificateExpiry;
}

void MQTTClientWrapper::setMutualSSLAuthentication(const QString& pathToCA, const QString& pathToPKey, const QString& pathToCertificate)
{
    d->pathToCA = pathToCA;
    d->pathToPKey = pathToPKey;
    d->pathToCertificate = pathToCertificate;

    QList<QSslCertificate> certificates = QSslCertificate::fromPath(pathToCertificate, QSsl::Pem);
    d->clientCertificateExpiry = certificates.value(0).expiryDate();

    // Instead of doing strdup, we let Qt's implicit sharing manage the stack for us
    QByteArray privateKey = d->pathToPKey.toLatin1();
    QByteArray trustStore = d->pathToCA.toLatin1();
    QByteArray keyStore = d->pathToCertificate.toLatin1();
}

void MQTTClientWrapper::setPublishQoS(MQTTClientWrapper::MQTTQoS qos)
{

    d->publishQoS = qos;
}

void MQTTClientWrapper::setSubscribeQoS(MQTTClientWrapper::MQTTQoS qos)
{

    d->subscribeQoS = qos;
}

void MQTTClientWrapper::setCleanSession(bool cleanSession)
{

    d->cleanSession = cleanSession;
}

void MQTTClientWrapper::setKeepAlive(quint64 seconds)
{

    d->keepAlive = seconds;
}

void MQTTClientWrapper::setIgnoreSslErrors(bool ignoreSslErrors)
{

    d->ignoreSslErrors = ignoreSslErrors;
}

void MQTTClientWrapper::setLastWill(const QByteArray& topic, const QByteArray& message, MQTTClientWrapper::MQTTQoS qos, bool retained)
{

    d->lwtMessage = message;
    d->lwtTopic = topic;
    d->lwtQos = qos;
    d->lwtRetained = retained;
}

QByteArray MQTTClientWrapper::hardwareId() const
{

    return d->hardwareId;
}

QByteArray MQTTClientWrapper::rootClientTopic() const
{

    return d->hardwareId;
}

bool MQTTClientWrapper::sessionPresent() const
{

    return d->sessionPresent;
}

void MQTTClientWrapper::initImpl()
{
    // Check the connack timeout and propagate it
    connect(d->connackTimer, SIGNAL(timeout()), this, SIGNAL(connackTimeout()));
    // Connect the signals received from the callback thread
    connect(this, SIGNAL(connectionStarted()), d->connackTimer, SLOT(start()));
    connect(this, SIGNAL(connackReceived()), d->connackTimer, SLOT(stop()));

    // Initialize stuff
    d->mosquitto = new HyperdriveMosquittoClient(d, d->hardwareId.constData(), d->cleanSession);

    // SSL
    if (!d->pathToCA.isEmpty() && !d->pathToPKey.isEmpty() && !d->pathToCertificate.isEmpty()) {
        // Instead of doing strdup, we let Qt's implicit sharing manage the stack for us
        QByteArray privateKey = d->pathToPKey.toLatin1();
        QByteArray trustStore = d->pathToCA.toLatin1();
        QByteArray keyStore = d->pathToCertificate.toLatin1();
        // Configure mutual SSL authentication.
        qDebug() << "Setting TLS!" << trustStore << keyStore << privateKey;
        d->mosquitto->tls_set(trustStore.constData(), NULL, keyStore.constData(), privateKey.constData());
        if (d->ignoreSslErrors) {
            d->mosquitto->tls_opts_set(0);
        } else {
            d->mosquitto->tls_opts_set(1);
        }
    }

    // Always successful
    mosqpp::lib_init();

    qWarning() << "Mosquitto is up!";

    setReady();
}

bool MQTTClientWrapper::connectToBroker()
{
    switch (d->status) {
        // We are already connecting
        case ConnectingStatus:
        case ConnectedStatus:
        case ReconnectingStatus: {
            return true;
        }

        case DisconnectedStatus:
        case DisconnectingStatus: {
            int rc;

            qDebug() << "Starting mosquitto connection" << d->serverUrl.host() << d->serverUrl.port();

            if ((rc = d->mosquitto->connect_async(qstrdup(d->serverUrl.host().toLatin1().constData()), d->serverUrl.port(), 60)) != MOSQ_ERR_SUCCESS) {
                qWarning() << "Could not initiate async mosquitto connection! Return code " << rc;
                Q_EMIT connectionFailed();
                return false;
            }
            if (d->mosquitto->loop_start() != MOSQ_ERR_SUCCESS) {
                qWarning() << "Could not initiate async mosquitto loop! Something is beyond broken!";
                return false;
            }

            d->setStatus(MQTTClientWrapper::ConnectingStatus);
            qDebug() << "Started mosquitto connection";

            d->connackTimer->start();

            return true;
        }

        default: {
            qWarning() << "Unknown status in connect, something is really wrong";
            return false;
        }
    }
}

bool MQTTClientWrapper::disconnectFromBroker()
{


    switch (d->status) {
        // We are already disconnecting
        case DisconnectingStatus:
        case DisconnectedStatus: {
            return true;
        }

        case ConnectingStatus:
        case ConnectedStatus:
        case ReconnectingStatus: {
            int rc;

            d->connackTimer->stop();

            if ((rc = d->mosquitto->disconnect()) != MOSQ_ERR_SUCCESS) {
                if (rc == MOSQ_ERR_NO_CONN) {
                    qWarning() << "Trying to disconnect, but not connected to a broker";
                    d->setStatus(MQTTClientWrapper::DisconnectedStatus);
                    return true;
                } else {
                    qWarning() << "Failed to start disconnect, return code" << rc;
                    return false;
                }
            }
            d->setStatus(MQTTClientWrapper::DisconnectingStatus);
            return true;
        }
        default: {
            qWarning() << "Unknown status in disconnect, something is really wrong";
            return false;
        }
    }
}

int MQTTClientWrapper::publish(const QByteArray& topic, const QByteArray& payload, MQTTQoS lqos, bool retained)
{


    if (Q_UNLIKELY(!d->mosquitto)) {
        qWarning() << "Attempted to call publish before initializing the client!";
        return -1;
    }

    int rc;
    int qos = lqos == DefaultQoS ? d->publishQoS : (int)lqos;
    int mid;

    if ((rc = d->mosquitto->publish(&mid, topic.constData(), payload.length(), const_cast<char*>(payload.data()), qos, retained)) != MOSQ_ERR_SUCCESS) {
        qWarning() << "Failed to start sendMessage, return code " << rc;
        return -rc;
    }

    return mid;
}

void MQTTClientWrapper::subscribe(const QByteArray& topic, MQTTQoS subQoS)
{


    if (Q_UNLIKELY(!d->mosquitto)) {
        qWarning() << "Attempted to call subscribe before initializing the client!";
        return;
    }

    int rc;
    int qos;
    if (subQoS == DefaultQoS) {
        qos = d->publishQoS;
    } else {
        qos = (int)subQoS;
    }

    if ((rc = d->mosquitto->subscribe(NULL, topic.constData(), qos)) != MOSQ_ERR_SUCCESS) {
        qWarning() << "Failed to start subscribe, return code " << rc;
    }
}

}
