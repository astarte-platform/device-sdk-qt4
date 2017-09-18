#include "transportdatabasemanager.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#define VERSION_VALUE 0

#define TARGET_VALUE 0
#define PAYLOAD_VALUE 1

#define EXPIRY_VALUE 0

#define ID_VALUE 0
#define CACHEMESSAGE_VALUE 1

namespace TransportDatabaseManager {

bool ensureDatabase(const QString &dbPath, const QString &migrationsDirPath)
{
    if (QSqlDatabase::database().isValid()) {
        return true;
    }

    if (dbPath.isEmpty() || migrationsDirPath.isEmpty()) {
        qWarning() << "You have to provide dbPath and migrationsDirPath in the first call to ensureDatabase";
        return false;
    }

    // Let's create our connection.
    QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));

    // Does the directory exist? We have to create, or SQLITE will complain.
    QDir dbDir(QFileInfo(dbPath).dir());
    if (!dbDir.exists()) {
        qDebug() << "Creating path... " << dbDir.mkpath(dbDir.absolutePath());
    }

    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning() << "Could not open database!" << dbPath << db.lastError().text();
        return false;
    }

    QSqlQuery checkQuery(QLatin1String("pragma quick_check"));
    if (!checkQuery.exec()) {
        qWarning() << "Database" << dbPath << " is corrupted, deleting it and starting from a new one " << checkQuery.lastError();
        db.close();
        if (!QFile::remove(dbPath)) {
            qWarning() << "Can't remove database " << dbPath << ", giving up ";
            return false;
        }
        if (!db.open()) {
            qWarning() << "Can't reopen database " << dbPath << " after deleting it, giving up " << db.lastError().text();
            return false;
        }
    }

    QSqlQuery migrationQuery;

    // Ok. Let's query our migrations.
    QDir migrationsDir(migrationsDirPath);
    migrationsDir.setFilter(QDir::Files | QDir::NoSymLinks);
    migrationsDir.setSorting(QDir::Name);
    QHash< int, QString > migrations;
    int latestSchemaVersion = 0;
    Q_FOREACH (const QFileInfo &migration, migrationsDir.entryInfoList()) {
        latestSchemaVersion = migration.baseName().split(QLatin1Char('_')).first().toInt();
        migrations.insert(latestSchemaVersion, migration.absoluteFilePath());
    }

    // Query our schema table
    QSqlQuery schemaQuery(QLatin1String("SELECT version from schema_version"));
    int currentSchemaVersion = -1;
    while (schemaQuery.next()) {
        if (schemaQuery.value(VERSION_VALUE).toInt() == latestSchemaVersion) {
            // Yo.
            return true;
        } else if (schemaQuery.value(VERSION_VALUE).toInt() > latestSchemaVersion) {
            // Yo.
            qWarning() << "Something is weird!! Our schema version is higher than available migrations? Ignoring and hoping for the best...";
            return true;
        }

        // Let's point out which one we need.
        currentSchemaVersion = schemaQuery.value(VERSION_VALUE).toInt();
    }

    // We need to migrate.
    qWarning() << "Found these migrations:" << migrations;
    qWarning() << "Latest schema version is " << latestSchemaVersion;

    // If we got here, we might need to create the schema table first.
    if (currentSchemaVersion < 0) {
        qDebug() << "Creating schema_version table...";
        if (!migrationQuery.exec(QLatin1String("CREATE TABLE schema_version(version integer)"))) {
            qWarning() << "Could not create schema_version!!" << migrationQuery.lastError();
            return false;
        }
        if (!migrationQuery.exec(QLatin1String("INSERT INTO schema_version (version) VALUES (0)"))) {
            qWarning() << "Could not create schema_version!!" << migrationQuery.lastError();
            return false;
        }
    }

    qDebug() << "Now performing migrations" << currentSchemaVersion << latestSchemaVersion;
    // Perform migrations.
    for (++currentSchemaVersion; currentSchemaVersion <= latestSchemaVersion; ++currentSchemaVersion) {
        if (!migrations.contains(currentSchemaVersion)) {
            continue;
        }

        // Apply migration
        QFile migrationFile(migrations.value(currentSchemaVersion));
        if (migrationFile.open(QIODevice::ReadOnly)) {
            QString queryString = QTextStream(&migrationFile).readAll();
            if (!migrationQuery.exec(queryString)) {
                qWarning() << "Could not execute migration" << currentSchemaVersion << migrationQuery.lastError();
                return false;
            }
        }

        // Update schema version
        if (!migrationQuery.exec(QString("UPDATE schema_version SET version=%1").arg(currentSchemaVersion))) {
            qWarning() << "Could not update schema_version!! This error is critical, your database is compromised!!" << migrationQuery.lastError();
            return false;
        }

        qDebug() << "Migration performed" << currentSchemaVersion;
    }

    // We're set!
    return true;
}

bool Transactions::insertPersistentEntry(const QByteArray &target, const QByteArray &payload)
{
    if (!ensureDatabase()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("INSERT INTO persistent_entries (target, payload) "
                                 "VALUES (:target, :payload)"));
    query.bindValue(QLatin1String(":target"), QLatin1String(target));
    query.bindValue(QLatin1String(":payload"), payload);

    if (!query.exec()) {
        qWarning() << "Insert persistent entry query failed!" << query.lastError();
        return false;
    }

    return true;
}

bool Transactions::updatePersistentEntry(const QByteArray &target, const QByteArray &payload)
{
    if (!ensureDatabase()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("UPDATE persistent_entries SET payload=:payload "
                                 "WHERE target=:target"));
    query.bindValue(QLatin1String(":target"), QLatin1String(target));
    query.bindValue(QLatin1String(":payload"), payload);

    if (!query.exec()) {
        qWarning() << "Update persistent entry query failed!" << query.lastError();
        return false;
    }

    return true;
}

bool Transactions::deletePersistentEntry(const QByteArray &target)
{
    if (!ensureDatabase()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("DELETE FROM persistent_entries WHERE target=:target"));
    query.bindValue(QLatin1String(":target"), QLatin1String(target));

    if (!query.exec()) {
        qWarning() << "Delete persistent entry " << target << " query failed!" << query.lastError();
        return false;
    }

    return true;
}

QHash<QByteArray, QByteArray> Transactions::allPersistentEntries()
{
    QHash<QByteArray, QByteArray> ret;

    if (!ensureDatabase()) {
        return ret;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("SELECT target, payload FROM persistent_entries"));

    if (!query.exec()) {
        qWarning() << "All persistent entries query failed!" << query.lastError();
        return ret;
    }

    while (query.next()) {
        ret.insert(query.value(TARGET_VALUE).toByteArray(), query.value(PAYLOAD_VALUE).toByteArray());
    }

    return ret;
}

int Transactions::insertCacheMessage(const Astarte::CacheMessage &cacheMessage, const QDateTime &expiry)
{
    if (!ensureDatabase()) {
        return -1;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("INSERT INTO cachemessages (cachemessage, expiry) "
                                 "VALUES (:cachemessage, :expiry)"));
    query.bindValue(QLatin1String(":cachemessage"), cacheMessage.serialize());
    query.bindValue(QLatin1String(":expiry"), expiry);

    if (!query.exec()) {
        qWarning() << "Insert Astarte::CacheMessage query failed!" << query.lastError();
        return -1;
    }

    return query.lastInsertId().toInt();
}

bool Transactions::deleteCacheMessage(int id)
{
    if (!ensureDatabase()) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QLatin1String("DELETE FROM cachemessages WHERE id=:id"));
    query.bindValue(QLatin1String(":id"), id);

    if (!query.exec()) {
        qWarning() << "Delete Astarte::CacheMessage query failed!" << query.lastError();
        return false;
    }

    return true;
}

QList<Astarte::CacheMessage> Transactions::allCacheMessages()
{
    QList<Astarte::CacheMessage> ret;

    if (!ensureDatabase()) {
        return ret;
    }

    // Housekeeping: delete expired Astarte::CacheMessages
    QSqlQuery query;
    query.prepare(QLatin1String("DELETE FROM cachemessages WHERE expiry < :now"));
    query.bindValue(QLatin1String(":now"), QDateTime::currentDateTime());

    if (!query.exec()) {
        qWarning() << "Delete expired Astarte::CacheMessages query failed!" << query.lastError();
        return ret;
    }

    query.prepare(QLatin1String("SELECT id, cachemessage FROM cachemessages"));

    if (!query.exec()) {
        qWarning() << "All Astarte::CacheMessages query failed!" << query.lastError();
        return ret;
    }

    while (query.next()) {
        Astarte::CacheMessage c = Astarte::CacheMessage::fromBinary(query.value(CACHEMESSAGE_VALUE).toByteArray());
        c.addAttribute("dbId", QByteArray::number(query.value(ID_VALUE).toInt()));
        ret.append(c);
    }

    return ret;
}

}
