#ifndef ASTARTE_TRANSPORT_CACHE_H
#define ASTARTE_TRANSPORT_CACHE_H

#include "utils/hemeraasyncinitobject.h"

#include "cachemessage.h"

namespace Astarte {

class TransportCache : public Hemera::AsyncInitObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TransportCache)

public:
    static TransportCache *instance();

    static void setPersistencyDir(const QString &persistencyDir);

    virtual ~TransportCache();

public Q_SLOTS:
    void insertOrUpdatePersistentEntry(const QByteArray &target, const QByteArray &payload);
    void removePersistentEntry(const QByteArray &target);

    QByteArray persistentEntry(const QByteArray &target) const;
    QHash< QByteArray, QByteArray > allPersistentEntries() const;

    bool isCached(const QByteArray &target) const;

    void addInFlightEntry(int messageId, Astarte::CacheMessage message);
    Astarte::CacheMessage takeInFlightEntry(int messageId);
    void resetInFlightEntries();

    int addRetryEntry(Astarte::CacheMessage message);
    void removeRetryEntry(int messageId);

    Astarte::CacheMessage takeRetryEntry(int id);
    QList<int> allRetryIds() const;

    void removeFromDatabase(const Astarte::CacheMessage &message);


protected:
    virtual void initImpl();
    virtual void timerEvent(QTimerEvent *event);

private:
    explicit TransportCache(QObject *parent = 0);

    void insertIntoDatabaseIfNotPresent(Astarte::CacheMessage &message);

    bool ensureDatabase();

    bool m_dbOk;

    class Private;
    Private * const d;
};

}

#endif // ASTARTE_TRANSPORT_CACHE_H


