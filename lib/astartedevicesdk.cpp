/*
 * Copyright (C) 2017 Ispirata Srl
 *
 * This file is part of Astarte.
 * Astarte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Astarte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Astarte.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "astartedevicesdk_p.h"

#include "astarteinterface.h"

#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include "internal/transport.h"
#include "internal/producerabstractinterface.h"

#include "utils/astartegenericconsumer.h"
#include "utils/astartegenericproducer.h"
#include "utils/validateinterfaceoperation.h"
#include "utils/utils.h"

using namespace rapidjson;

AstarteDeviceSDK::AstarteDeviceSDK(const QString &configurationPath, const QString &interfacesDir,
                                   const QByteArray &hardwareId, QObject *parent)
    : AsyncInitObject(parent)
    , d(new Private(this))
{
    d->hardwareId = hardwareId;
    d->interfacesDir = interfacesDir;
    d->configurationPath = configurationPath;
}

AstarteDeviceSDK::~AstarteDeviceSDK()
{
}

void AstarteDeviceSDK::initImpl()
{
    d->astarteTransport = new Astarte::Transport(d->configurationPath, d->hardwareId, this);
    if (!d->astarteTransport->init()->synchronize()) {
        setInitError("transport", "op->errorMessage()");
    }

    d->loadInterfaces();

    setReady();
}

void AstarteDeviceSDK::Private::loadInterfaces()
{
    QHash< QByteArray, AstarteInterface > introspection;

    QDir interfacesDirectory(interfacesDir);
    interfacesDirectory.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList nameFilters = QStringList() << QLatin1String("*.json");
    interfacesDirectory.setNameFilters(nameFilters);

    QList<QFileInfo> interfaceFiles = interfacesDirectory.entryInfoList();

    // Load schema
    QFile schemaFile(QString("%1/interface.json").arg(
                QLatin1String("/usr/share/astarte-sdk")));
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        q->setInitError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())",
                        QString("Schema file %1 does not exist").arg(schemaFile.fileName()));
        return;
    }

    Document sd;
    if (sd.Parse(schemaFile.readAll().constData()).HasParseError()) {
        q->setInitError("wrong format", "could not parse schema!");
        return;
    }
    SchemaDocument schema(sd); // Compile a Document to SchemaDocument
    // sd is no longer needed here.
    SchemaValidator validator(schema);

    Q_FOREACH (const QFileInfo &fileInfo, interfaceFiles) {
        QString filePath = fileInfo.absoluteFilePath();
        qDebug() << "Loading interface " << filePath;

        QFile interfaceFile(filePath);
        if (!interfaceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Error opening interface file " << filePath;
            continue;
        }

        Document doc;
        doc.Parse(interfaceFile.readAll().constData());

        // Check the validation result
        if (doc.Accept(validator)) {
            AstarteInterface interface = AstarteInterface::fromJson(doc);
            introspection.insert(interface.interface(), interface);
            switch (interface.interfaceQuality()) {
                case AstarteInterface::Producer:
                    createProducer(interface, doc);
                    break;
                case AstarteInterface::Consumer:
                    createConsumer(interface, doc);
                    break;
                default:
                    qWarning() << "Invalid interface quality";
            }
            qDebug() << "Interface loaded " << interface.interface();
        } else {
            qWarning() << "Error loading interface " << filePath
                                          << ". Skipping it";
        }
    }

    astarteTransport->setIntrospection(introspection);
}

void AstarteDeviceSDK::Private::createConsumer(const AstarteInterface &interface, const rapidjson::Document &consumerObject)
{
    QHash<QByteArray, QByteArrayList> mappingToTokens;
    QHash<QByteArray, QVariant::Type> mappingToType;
    QHash<QByteArray, bool> mappingToAllowUnset;

    for (SizeType i = 0; i < consumerObject["mappings"].Size(); i++) {
        rapidjson::Value::ConstObject mappingObj = consumerObject["mappings"][i].GetObject();

        QByteArray path = mappingObj["path"].GetString();
        QByteArrayList tokens = path.mid(1).split('/');
        mappingToTokens.insert(path, tokens);

        QString typeString = mappingObj["type"].GetString();
        mappingToType.insert(path, typeStringToVariantType(typeString));

        if (interface.interfaceType() == AstarteInterface::Properties && mappingObj.HasMember("allow_unset")) {
            bool allowUnset = mappingObj["allow_unset"].GetBool();
            mappingToAllowUnset.insert(path, allowUnset);
        }
    }

    AstarteGenericConsumer *consumer = new AstarteGenericConsumer(interface.interface(), astarteTransport, q);
    consumer->setMappingToTokens(mappingToTokens);
    consumer->setMappingToType(mappingToType);
    consumer->setMappingToAllowUnset(mappingToAllowUnset);

    consumers.insert(interface.interface(), consumer);
    qDebug() << "Consumer for interface " << interface.interface() << " successfully initialized";
}

void AstarteDeviceSDK::Private::createProducer(const AstarteInterface &interface, const rapidjson::Document &producerObject)
{
    QHash<QByteArray, QByteArrayList> mappingToTokens;
    QHash<QByteArray, QVariant::Type> mappingToType;
    QHash<QByteArray, Retention> mappingToRetention;
    QHash<QByteArray, Reliability> mappingToReliability;
    QHash<QByteArray, int> mappingToExpiry;

    for (SizeType i = 0; i < producerObject["mappings"].Size(); i++) {
        rapidjson::Value::ConstObject mappingObj = producerObject["mappings"][i].GetObject();

        QByteArray path = mappingObj["path"].GetString();
        QByteArrayList tokens = path.mid(1).split('/');
        mappingToTokens.insert(path, tokens);

        QString typeString = mappingObj["type"].GetString();
        mappingToType.insert(path, typeStringToVariantType(typeString));

        if (interface.interfaceType() == AstarteInterface::DataStream) {
            if (mappingObj.HasMember("retention")) {
                QString retention = mappingObj["retention"].GetString();
                mappingToRetention.insert(path, retentionStringToRetention(retention));
            }
            if (mappingObj.HasMember("reliability")) {
                QString reliability = mappingObj["reliability"].GetString();
                mappingToReliability.insert(path, reliabilityStringToReliability(reliability));
            }
            if (mappingObj.HasMember("expiry")) {
                int expiry = mappingObj["expiry"].GetInt();
                mappingToExpiry.insert(path, expiry);
            }
        }
    }

    AstarteGenericProducer *producer = new AstarteGenericProducer(interface.interface(), interface.interfaceType(),
                                                                  astarteTransport, q);
    producer->setMappingToTokens(mappingToTokens);
    producer->setMappingToType(mappingToType);
    producer->setMappingToRetention(mappingToRetention);
    producer->setMappingToReliability(mappingToReliability);
    producer->setMappingToExpiry(mappingToExpiry);

    producers.insert(interface.interface(), producer);
    qDebug() << "Producer for interface " << interface.interface() << " successfully initialized";
}

QVariant::Type AstarteDeviceSDK::Private::typeStringToVariantType(const QString &typeString) const
{
    QVariant::Type dataType = QVariant::Invalid;
    if (typeString == QLatin1String("integer")) {
        dataType = QVariant::Int;
    } else if (typeString == QLatin1String("longinteger")) {
        dataType = QVariant::LongLong;
    } else if (typeString == QLatin1String("double")) {
        dataType = QVariant::Double;
    } else if (typeString == QLatin1String("datetime")) {
        dataType = QVariant::DateTime;
    } else if (typeString == QLatin1String("string")) {
        dataType = QVariant::String;
    } else if (typeString == QLatin1String("boolean")) {
        dataType = QVariant::Bool;
    } else if (typeString == QLatin1String("binary")) {
        dataType = QVariant::ByteArray;
    } else {
        qWarning() << QString("Type %1 unspecified!").arg(typeString);
    }

    return dataType;
}

Retention AstarteDeviceSDK::Private::retentionStringToRetention(const QString &retentionString) const
{
    if (retentionString == QLatin1String("stored")) {
        return Stored;
    } else if (retentionString == QLatin1String("volatile")) {
        return Volatile;
    } else {
        return Discard;
    }
}

Reliability AstarteDeviceSDK::Private::reliabilityStringToReliability(const QString &reliabilityString) const
{
    if (reliabilityString == QLatin1String("unique")) {
        return Unique;
    } else if (reliabilityString == QLatin1String("guaranteed")) {
        return Guaranteed;
    } else {
        return Unreliable;
    }
}

bool AstarteDeviceSDK::sendData(const QByteArray &interface, const QByteArray &path, const QVariant &value, const QVariantHash &metadata)
{
    return sendData(interface, path, value, QDateTime(), metadata);
}

bool AstarteDeviceSDK::sendData(const QByteArray &interface, const QByteArray &path, const QVariant &value, const QDateTime &timestamp,
                                const QVariantHash &metadata)
{
    if (!d->producers.contains(interface)) {
        qWarning() << "No producers for interface " << interface;
        return false;
    }

    return d->producers.value(interface)->sendData(value, path, timestamp, metadata);
}

bool AstarteDeviceSDK::sendData(const QByteArray &interface, const QVariantHash &value, const QVariantHash &metadata)
{
    return sendData(interface, value, QDateTime(), metadata);
}

bool AstarteDeviceSDK::sendData(const QByteArray &interface, const QVariantHash &value, const QDateTime &timestamp,
                                const QVariantHash &metadata)
{
    if (!d->producers.contains(interface)) {
        qWarning() << "No producers for interface " << interface;
        return false;
    }
    // Verify mappings
    QHash< QByteArray, QVariant::Type > mappingToType = d->producers.value(interface)->mappingToType();
    if (mappingToType.size() != value.size()) {
        qWarning() << "You have to provide exactly all the values of the aggregated interface!";
        return false;
    }

    QStringList pathTokens = value.constBegin().key().mid(1).split('/');
    pathTokens.removeLast();

    QString trailingPath;
    if (pathTokens.size() > 0) {
        trailingPath = "/" + pathTokens.join("/");
    }

    // There's a number of checks we have to do. Let's build a map first
    QHash<QByteArray, QByteArrayList> tokens;
    for (QVariantHash::const_iterator i = value.constBegin(); i != value.constEnd(); ++i) {
        QByteArrayList singleTokens;
        Q_FOREACH (const QString &token, i.key().mid(1).split('/')) {
            singleTokens.append(token.toLatin1());
        }
        tokens.insert(i.key().toLatin1(), singleTokens);
    }
    if (!Utils::verifySplitTokenMatch(tokens, d->producers.value(interface)->mappingToTokens())) {
        qWarning() << "Provided hash does not match interface definition!";
        return false;
    }

    QVariantHash normalizedValues;
    for (QVariantHash::const_iterator i = value.constBegin(); i != value.constEnd(); ++i) {
        /*if (mappingToType.value(i.key().toLatin1()) != i.value().type()) {
            qWarning() << "Type mismatch for property" << i.key() << "! Got" << i.value().type() << ", expected" << mappingToType.value(i.key().toLatin1());
            return false;
        }*/
        if (!trailingPath.isEmpty() && !i.key().startsWith(trailingPath)) {
            qWarning() << "Your path is malformed - this probably means you mistyped your parameters." << i.key() << "was expected to start with" << trailingPath;
            return false;
        }
        QString normalizedPath = i.key();
        normalizedPath.remove(0, trailingPath.size() + 1);
        normalizedValues.insert(normalizedPath, i.value());
    }
    // If we got here, verification was ok. Let's go.

    return d->producers.value(interface)->sendData(normalizedValues, trailingPath.toLatin1(), timestamp, metadata);
}

void AstarteDeviceSDK::Private::unsetValue(const QByteArray &interface, const QByteArray &path)
{
    Q_EMIT q->unsetReceived(interface, path);
}

void AstarteDeviceSDK::Private::receiveValue(const QByteArray &interface, const QByteArray &path, const QVariant &value)
{
    Q_EMIT q->dataReceived(interface, path, value);
}
