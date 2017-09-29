#include "hemeraasyncinitobject_p.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>

namespace Hemera
{

InitObjectOperation::InitObjectOperation(AsyncInitObject *parent)
    : Operation(parent)
    , m_object(parent)
{
    m_object->d_h_ptr->initOperation = this;
}

InitObjectOperation::~InitObjectOperation()
{
}

void InitObjectOperation::startImpl()
{
    QTimer::singleShot(0, m_object, SLOT(initHook()));
}

void AsyncInitObjectPrivate::initHook()
{
    Q_Q(AsyncInitObject);

    QTimer::singleShot(0, q, SLOT(initImpl()));
}

void AsyncInitObjectPrivate::postInitHook()
{
    Q_Q(AsyncInitObject);

    QTimer::singleShot(0, q, SLOT(finalizeInit()));
}

void AsyncInitObjectPrivate::finalizeInit()
{
    Q_Q(AsyncInitObject);

    initOperation.data()->setFinished();
    Q_EMIT q->ready();
}

AsyncInitObject::AsyncInitObject(QObject* parent)
    : QObject(parent)
    , d_h_ptr(new AsyncInitObjectPrivate(this))
{
}

AsyncInitObject::AsyncInitObject(Hemera::AsyncInitObjectPrivate& dd, QObject* parent)
    : QObject(parent)
    , d_h_ptr(&dd)
{
}

AsyncInitObject::~AsyncInitObject()
{
    delete d_h_ptr;
}

Hemera::Operation *AsyncInitObject::init()
{
    if (isReady()) {
        qWarning() << tr("Trying to initialize, but the object is already ready! Doing nothing.");
        return 0;
    }

    Q_D(AsyncInitObject);

    if (d->initOperation.isNull()) {
        return new InitObjectOperation(this);
    } else {
        return d->initOperation.data();
    }
}

bool AsyncInitObject::hasInitError() const
{
    Q_D(const AsyncInitObject);

    return !d->errorName.isEmpty();
}

QString AsyncInitObject::initError() const
{
    Q_D(const AsyncInitObject);

    return d->errorName;
}

QString AsyncInitObject::initErrorMessage() const
{
    Q_D(const AsyncInitObject);

    return d->errorMessage;
}

bool AsyncInitObject::isReady() const
{
    Q_D(const AsyncInitObject);

    return d->ready;
}

void AsyncInitObject::setReady()
{
    Q_D(AsyncInitObject);

    d->ready = true;
    d->postInitHook();
}

void AsyncInitObject::setParts(uint parts)
{
    Q_D(AsyncInitObject);

    d->parts = parts;
}

void AsyncInitObject::setOnePartIsReady()
{
    Q_D(AsyncInitObject);

    // We might as well have failed already.
    if (hasInitError()) {
        return;
    }

    if (d->parts == 0) {
        setInitError("Literals::literal(Literals::Errors::badRequest())",
                     tr("Called setOnePartIsReady on an object without parts, or called when all parts are already ready."));
        return;
    }

    --d->parts;

    if (d->parts == 0) {
        // Yay!
        setReady();
    }
}

void AsyncInitObject::setInitError(const QString &errorName, const QString &message)
{
    Q_D(AsyncInitObject);

    d->errorName = errorName;
    d->errorMessage = message;
    d->initOperation.data()->setFinishedWithError(errorName, message);
}

}

#include "moc_hemeraasyncinitobject.cpp"
