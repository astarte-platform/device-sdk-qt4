#ifndef TRANSPORT_DATABASE_MANAGER_H
#define TRANSPORT_DATABASE_MANAGER_H

#include "internal/cachemessage.h"

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QString>

namespace TransportDatabaseManager
{
    bool ensureDatabase(const QString &dbPath = QString(), const QString &migrationsDirPath = QString());

namespace Transactions
{
    bool insertPersistentEntry(const QByteArray &target, const QByteArray &payload);
    bool updatePersistentEntry(const QByteArray &target, const QByteArray &payload);
    bool deletePersistentEntry(const QByteArray &target);
    QHash<QByteArray, QByteArray> allPersistentEntries();

    int insertCacheMessage(const Astarte::CacheMessage &cacheMessage, const QDateTime &expiry = QDateTime());
    bool deleteCacheMessage(int id);
    QList<Astarte::CacheMessage> allCacheMessages();
}

}

#endif
