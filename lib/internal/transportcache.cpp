#include "transportcache.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTimerEvent>

#include "astarteinterface.h"
#include "utils/transportdatabasemanager.h"

#include "producerabstractinterface.h"

namespace Astarte {

class TransportCache::Private
{
public:
    QHash< QByteArray, QByteArray > persistentEntries;
    QHash< int, CacheMessage> inFlightEntries;
    QHash< int, CacheMessage > retryEntries;
    QHash< int, int > retryTimerToId;
    int retryIdCounter;

    Private()
    {
        retryIdCounter = 0;
    }
};

static TransportCache* s_instance;

static QString s_persistencyDir;

TransportCache::TransportCache(QObject *parent)
    : Hemera::AsyncInitObject(parent)
    , m_dbOk(false)
    , d(new Private)
{
}

TransportCache *TransportCache::instance()
{
    if (Q_UNLIKELY(!s_instance)) {
        s_instance = new TransportCache();
    }

    return s_instance;
}

TransportCache::~TransportCache()
{
    delete d;
}

void TransportCache::initImpl()
{
    if (ensureDatabase()) {

        d->persistentEntries = TransportDatabaseManager::Transactions::allPersistentEntries();
        QList<CacheMessage> dbMessages = TransportDatabaseManager::Transactions::allCacheMessages();
        Q_FOREACH (const CacheMessage &message, dbMessages) {
           d->retryEntries.insert(d->retryIdCounter++, message);
        }
        setReady();
    } else {
        setInitError("Hemera::Literals::literal(Hemera::Literals::Errors::failedRequest())", QLatin1String("Could not open the persistence database"));
    }
}

void TransportCache::setPersistencyDir(const QString &persistencyDir)
{
    s_persistencyDir = persistencyDir;
}

bool TransportCache::ensureDatabase()
{
    if (!m_dbOk) {
        m_dbOk = TransportDatabaseManager::ensureDatabase(QString("%1/persistence.db").arg(s_persistencyDir),
                                                          QString("%1/db/migrations").arg(QLatin1String("/usr/share/astarte-sdk")));
    }
    return m_dbOk;
}

void TransportCache::insertOrUpdatePersistentEntry(const QByteArray &target, const QByteArray &payload)
{
    ensureDatabase();
    if (d->persistentEntries.contains(target)) {
        TransportDatabaseManager::Transactions::updatePersistentEntry(target, payload);
    } else {
        TransportDatabaseManager::Transactions::insertPersistentEntry(target, payload);
    }
    d->persistentEntries.insert(target, payload);
}

void TransportCache::removePersistentEntry(const QByteArray &target)
{
    ensureDatabase();
    TransportDatabaseManager::Transactions::deletePersistentEntry(target);
    d->persistentEntries.remove(target);
}

bool TransportCache::isCached(const QByteArray &target) const
{
    return d->persistentEntries.contains(target);
}

QByteArray TransportCache::persistentEntry(const QByteArray &target) const
{
    return d->persistentEntries.value(target);
}

QHash< QByteArray, QByteArray > TransportCache::allPersistentEntries() const
{
    return d->persistentEntries;
}

void TransportCache::addInFlightEntry(int messageId, CacheMessage message)
{
    if (message.attributes().value("retention").toInt() == static_cast<int>(Discard)) {
        // QoS 0, discard it
        return;
    }

    if (message.interfaceType() == AstarteInterface::Properties ||
        message.attributes().value("retention").toInt() == static_cast<int>(Stored)) {

        insertIntoDatabaseIfNotPresent(message);
    }
    d->inFlightEntries.insert(messageId, message);
}

CacheMessage TransportCache::takeInFlightEntry(int messageId)
{
    removeFromDatabase(d->inFlightEntries.value(messageId));
    return d->inFlightEntries.take(messageId);
}

void TransportCache::resetInFlightEntries()
{
    Q_FOREACH (CacheMessage c, d->inFlightEntries.values()) {
        addRetryEntry(c);
    }
    d->inFlightEntries.clear();
}

void TransportCache::insertIntoDatabaseIfNotPresent(CacheMessage &message)
{
    if (!message.hasAttribute("dbId")) {
        // We have to insert it in the db

        ensureDatabase();
        QDateTime absoluteExpiry;
        // Check if we don't have an absolute expiry
        if (!message.hasAttribute("absoluteExpiry")) {
            // If we actually have an expiry, convert it to an absolute one
            if (message.hasAttribute("expiry")) {
                int relativeExpiry = message.attribute("expiry").toInt();
                if (relativeExpiry > 0) {
                    absoluteExpiry = QDateTime::currentDateTime().addSecs(relativeExpiry);
                    message.addAttribute("absoluteExpiry", QByteArray::number(absoluteExpiry.toMSecsSinceEpoch()));
                    message.removeAttribute("expiry");
                }
            }
        } else {
            absoluteExpiry = QDateTime::fromMSecsSinceEpoch(message.attribute("absoluteExpiry").toLongLong());
        }

        int dbId = TransportDatabaseManager::Transactions::insertCacheMessage(message, absoluteExpiry);
        message.addAttribute("dbId", QByteArray::number(dbId));
    }
}

int TransportCache::addRetryEntry(CacheMessage message)
{
    if (message.attributes().value("retention").toInt() == static_cast<int>(Discard)) {
        // QoS 0, discard it
        return -1;
    }
    if (message.interfaceType() == AstarteInterface::Properties ||
        message.attributes().value("retention").toInt() == static_cast<int>(Stored)) {

        insertIntoDatabaseIfNotPresent(message);
    }
    int id = d->retryIdCounter++;
    d->retryEntries.insert(id, message);

    int relativeExpiryms = 0;
    if (message.hasAttribute("absoluteExpiry")) {
        QDateTime absoluteExpiry = QDateTime::fromMSecsSinceEpoch(message.attribute("absoluteExpiry").toLongLong());
        relativeExpiryms = QDateTime::currentDateTime().msecsTo(absoluteExpiry);
    } else if (message.hasAttribute("expiry")) {
        relativeExpiryms = message.attribute("expiry").toInt() * 1000;
    }
    if (relativeExpiryms > 0) {
        int timerId = startTimer(relativeExpiryms);
        d->retryTimerToId.insert(timerId, id);
    }

    return id;
}

void TransportCache::removeRetryEntry(int id)
{
    removeFromDatabase(d->retryEntries.value(id));
    d->retryEntries.remove(id);
}

CacheMessage TransportCache::takeRetryEntry(int id)
{
    return d->retryEntries.take(id);
}

QList< int > TransportCache::allRetryIds() const
{
    return d->retryEntries.keys();
}

void TransportCache::removeFromDatabase(const CacheMessage &message)
{
    if (message.hasAttribute("dbId")) {
        ensureDatabase();
        TransportDatabaseManager::Transactions::deleteCacheMessage(message.attribute("dbId").toInt());
    }
}

void TransportCache::timerEvent(QTimerEvent *event)
{
    if (d->retryTimerToId.contains(event->timerId())) {
        int messageId = d->retryTimerToId.take(event->timerId());
        removeRetryEntry(messageId);
    }
}

}
