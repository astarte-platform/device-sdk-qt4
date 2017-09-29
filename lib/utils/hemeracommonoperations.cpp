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

#include "hemeracommonoperations.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>

#include <functional>

namespace Hemera {

class FailureOperation::Private
{
public:
    QString name;
    QString message;
};

FailureOperation::FailureOperation(const QString& errorName, const QString& errorMessage, QObject* parent)
    : Operation(parent)
    , d(new Private)
{
    d->name = errorName;
    d->message = errorMessage;
}

FailureOperation::~FailureOperation()
{
}

void FailureOperation::startImpl()
{
    setFinishedWithError(d->name, d->message);
}

/////////////////////

SuccessOperation::SuccessOperation(QObject* parent)
    : Operation(parent)
    , d(0)
{
}

SuccessOperation::~SuccessOperation()
{
}

void SuccessOperation::startImpl()
{
    setFinished();
}

/////////////////////

ObjectOperation::ObjectOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

ObjectOperation::~ObjectOperation()
{
}

/////////////////////

VariantOperation::VariantOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

VariantOperation::~VariantOperation()
{
}


/////////////////////

VariantMapOperation::VariantMapOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

VariantMapOperation::~VariantMapOperation()
{
}

/////////////////////

UIntOperation::UIntOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

UIntOperation::~UIntOperation()
{
}

BoolOperation::BoolOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

BoolOperation::~BoolOperation()
{
}

/////////////////////


ByteArrayOperation::ByteArrayOperation(QObject *parent)
    : Operation(parent),
      d(0)
{
}

ByteArrayOperation::~ByteArrayOperation()
{
}

/////////////////////


SSLCertificateOperation::SSLCertificateOperation(QObject *parent)
    : Operation(parent)
    , d(0)
{
}

SSLCertificateOperation::~SSLCertificateOperation()
{
}

/////////////////////

StringOperation::StringOperation(QObject* parent)
    : Operation(parent)
    , d(0)
{
}

StringOperation::~StringOperation()
{
}

/////////////////////

StringListOperation::StringListOperation(QObject* parent)
    : Operation(parent)
    , d(0)
{
}

StringListOperation::~StringListOperation()
{
}

/////////////////////

JsonOperation::JsonOperation(QObject* parent)
    : Operation(parent)
    , d(0)
{
}

JsonOperation::~JsonOperation()
{
}

/////////////////////

class UrlOperation::Private
{
public:
    QUrl result;
};

UrlOperation::UrlOperation(QObject* parent)
    : Operation(parent)
    , d(0)
{
}

UrlOperation::~UrlOperation()
{
    delete d;
}

}

#include "moc_hemeracommonoperations.cpp"
