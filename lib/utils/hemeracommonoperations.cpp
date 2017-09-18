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
