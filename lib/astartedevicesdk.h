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

#ifndef ASTARTEDEVICESDK_H
#define ASTARTEDEVICESDK_H

#include <QtCore/QObject>
#include <QtCore/QDateTime>
#include <QtCore/QHash>

#include <utils/hemeraasyncinitobject.h>
#include <astartedevicesdk_global.h>

typedef QList<QByteArray> QByteArrayList;

class AstarteGenericConsumer;
class AstarteGenericProducer;

class ASTARTEQT4SDKSHARED_EXPORT AstarteDeviceSDK : public Hemera::AsyncInitObject
{
    Q_OBJECT

public:
    AstarteDeviceSDK(const QString &configurationPath, const QString &interfacesDir,
                     const QByteArray &hardwareId, QObject *parent = 0);
    virtual ~AstarteDeviceSDK();

    bool sendData(const QByteArray &interface, const QByteArray &path, const QVariant &value, const QVariantHash &metadata);
    bool sendData(const QByteArray &interface, const QByteArray &path, const QVariant &value, const QDateTime &timestamp = QDateTime(),
                  const QVariantHash &metadata = QVariantHash());
    bool sendData(const QByteArray &interface, const QVariantHash &value, const QVariantHash &metadata);
    bool sendData(const QByteArray &interface, const QVariantHash &value, const QDateTime &timestamp = QDateTime(),
                  const QVariantHash &metadata = QVariantHash());

Q_SIGNALS:
    void unsetReceived(const QByteArray &interface, const QByteArray &path);
    void dataReceived(const QByteArray &interface, const QByteArray &path, const QVariant &value);

protected Q_SLOTS:

    void initImpl();

private:
    class Private;
    Private * const d;

    friend class AstarteGenericConsumer;
};

#endif
