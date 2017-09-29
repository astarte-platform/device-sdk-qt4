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

#ifndef HEMERA_COMMONOPERATIONS_H
#define HEMERA_COMMONOPERATIONS_H

#include "hemeraoperation.h"

#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

#include <rapidjson/document.h>

class QProcess;

namespace Hemera {

class FailureOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(FailureOperation)

public:
    explicit FailureOperation(const QString &errorName, const QString &errorMessage, QObject *parent = 0);
    virtual ~FailureOperation();

protected Q_SLOTS:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

class SuccessOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(SuccessOperation)

public:
    explicit SuccessOperation(QObject *parent = 0);
    virtual ~SuccessOperation();

protected Q_SLOTS:
    virtual void startImpl();

private:
    class Private;
    Private * const d;
};

class ObjectOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(ObjectOperation)

    Q_PROPERTY(QObject * result READ result)

public:
    virtual ~ObjectOperation();
    virtual QObject *result() const = 0;

protected:
    explicit ObjectOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};


class VariantOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(VariantOperation)

    Q_PROPERTY(QVariant result READ result)

public:
    virtual ~VariantOperation();
    virtual QVariant result() const = 0;

protected:
    explicit VariantOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class VariantMapOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(VariantMapOperation)

    Q_PROPERTY(QVariantMap result READ result)

public:
    virtual ~VariantMapOperation();
    virtual QVariantMap result() const = 0;

protected:
    explicit VariantMapOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class BoolOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(BoolOperation)

    Q_PROPERTY(bool result READ result)

public:
    virtual ~BoolOperation();
    virtual bool result() const = 0;

protected:
    BoolOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class UIntOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(UIntOperation)

    Q_PROPERTY(uint result READ result)

public:
    virtual ~UIntOperation();
    virtual uint result() const = 0;

protected:
    UIntOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class ByteArrayOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(ByteArrayOperation)

    Q_PROPERTY(QByteArray result READ result)

public:
    virtual ~ByteArrayOperation();
    virtual QByteArray result() const = 0;

protected:
    ByteArrayOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class SSLCertificateOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(SSLCertificateOperation)

    Q_PROPERTY(QByteArray privateKey READ privateKey)
    Q_PROPERTY(QByteArray certificate READ certificate)

public:
    virtual ~SSLCertificateOperation();
    virtual QByteArray privateKey() const = 0;
    virtual QByteArray certificate() const = 0;

protected:
    SSLCertificateOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class StringOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(StringOperation)

    Q_PROPERTY(QString result READ result)

public:
    virtual ~StringOperation();
    virtual QString result() const = 0;

protected:
    StringOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class StringListOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(StringListOperation)

    Q_PROPERTY(QStringList result READ result)

public:
    virtual ~StringListOperation();
    virtual QStringList result() const = 0;

protected:
    StringListOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class JsonOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(JsonOperation)

public:
    virtual ~JsonOperation();
    virtual rapidjson::Document result() const = 0;

protected:
    JsonOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

class UrlOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(UrlOperation)

    Q_PROPERTY(QUrl result READ result)

public:
    virtual ~UrlOperation();
    virtual QUrl result() const = 0;

protected:
    UrlOperation(QObject *parent = 0);

private:
    class Private;
    Private * const d;
};

}

#endif // HEMERA_DBUSVOIDOPERATION_H
