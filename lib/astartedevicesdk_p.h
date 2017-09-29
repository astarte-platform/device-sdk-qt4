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

#ifndef QT4SDK_P_H
#define QT4SDK_P_H

#include "astartedevicesdk.h"

#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"

#include "astarteinterface.h"

#include "internal/producerabstractinterface.h"

using namespace rapidjson;

class AstarteDeviceSDK::Private {
public:
    Private(AstarteDeviceSDK *q) : q(q) {}
    ~Private() {}

    AstarteDeviceSDK * const q;

    Astarte::Transport *astarteTransport;

    QString configurationPath;
    QString interfacesDir;

    QByteArray hardwareId;

    QHash<QByteArray, AstarteGenericProducer *> producers;
    QHash<QByteArray, AstarteGenericConsumer *> consumers;

    void loadInterfaces();

    void createProducer(const AstarteInterface &interface, const rapidjson::Document &producerObject);
    void createConsumer(const AstarteInterface &interface, const rapidjson::Document &consumerObject);

    QVariant::Type typeStringToVariantType(const QString &typeString) const;
    Retention retentionStringToRetention(const QString &retentionString) const;
    Reliability reliabilityStringToReliability(const QString &reliabilityString) const;

    void receiveValue(const QByteArray &interface, const QByteArray &path, const QVariant &value);
    void unsetValue(const QByteArray &interface, const QByteArray &path);
};

#endif // QT4SDK_P_H
