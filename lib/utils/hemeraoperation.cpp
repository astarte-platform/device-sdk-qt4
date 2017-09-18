#include "hemeraoperation_p.h"

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

namespace Hemera {

void OperationPrivate::emitFinished()
{
    Q_ASSERT(finished);
    Q_Q(Operation);
    Q_EMIT q->finished(q);
    // Delay deletion of 2 seconds to be safe
    QTimer::singleShot(2000, q, SLOT(deleteLater()));
}

void OperationPrivate::setStarted()
{
    Q_Q(Operation);
    started = true;
    Q_EMIT q->started(q);
}

Operation::Operation(QObject* parent)
    : QObject(parent)
    , d_ptr(new OperationPrivate(this))
{
    Q_D(Operation);
    d->genericInit(NoOptions);
}

Operation::Operation(ExecutionOptions options, QObject *parent)
    : QObject(parent)
    , d_ptr(new OperationPrivate(this))
{
    Q_D(Operation);
    d->genericInit(options);
}

Operation::Operation(OperationPrivate &dd, ExecutionOptions options, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(Operation);
    d->genericInit(options);
}

void OperationPrivate::genericInit(Operation::ExecutionOptions cOptions)
{
    Q_Q(Operation);
    options = cOptions;
    if (!(options & Operation::ExplicitStartOption)) {
        if (!(privateOptions & OperationPrivate::SetStartedExplicitly)) {
            QTimer::singleShot(0, q, SLOT(setStarted()));
        }
        QTimer::singleShot(0, q, SLOT(startImpl()));
    }
}

Operation::~Operation()
{
    if (!isFinished()) {
        qWarning() << tr("The operation is being deleted, but is not finished yet.");
    }
}

bool Operation::start()
{
    Q_D(Operation);

    if (!(d->started) && (d->options & ExplicitStartOption)) {
        if (!(d->privateOptions & OperationPrivate::SetStartedExplicitly)) {
            d->setStarted();
        }
        QTimer::singleShot(0, this, SLOT(startImpl()));
        return true;
    } else {
       return false;
    }
}

QString Operation::errorMessage() const
{
    Q_D(const Operation);
    return d->errorMessage;
}

QString Operation::errorName() const
{
    Q_D(const Operation);
    return d->errorName;
}

bool Operation::isError() const
{
    Q_D(const Operation);
    return (d->finished && !d->errorName.isEmpty());
}

bool Operation::isFinished() const
{
    Q_D(const Operation);
    return d->finished;
}

bool Operation::isValid() const
{
    Q_D(const Operation);
    return (d->finished && d->errorName.isEmpty());
}


bool Operation::isStarted() const
{
    Q_D(const Operation);
    return d->started;
}

void Operation::setFinished()
{
    Q_D(Operation);

    if (d->finished) {
        if (!d->errorName.isEmpty()) {
            qWarning() << this << tr("Trying to finish with success, but already failed with %2 : %3")
                                                       .arg(d->errorName, d->errorMessage);
        } else {
            qWarning() << this << tr("Trying to finish with success, but already succeeded");
        }
        return;
    }

    d->finished = true;
    Q_ASSERT(isValid());
    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

void Operation::setFinishedWithError(const QString& name, const QString& message)
{
    Q_D(Operation);

    if (d->finished) {
        if (!d->errorName.isEmpty()) {
            qWarning() << this << tr("Trying to fail with %1, but already failed with %2 : %3")
                                                        .arg(name, d->errorName, d->errorMessage);
        } else {
            qWarning() << this << tr("Trying to fail with %1, but already succeeded").arg(name);
        }
        return;
    }

    if (name.isEmpty()) {
        qWarning() << this << tr("should be given a non-empty error name");
        d->errorName = "Hemera::Literals::literal(Hemera::Literals::Errors::errorHandlingError())";
    } else {
        d->errorName = name;
    }

    d->errorMessage = message;
    d->finished = true;
    Q_ASSERT(isError());
    QTimer::singleShot(0, this, SLOT(emitFinished()));
}

bool Operation::synchronize(int timeout)
{
    if (isFinished()) {
        return !isError();
    }

    QEventLoop e;
    connect(this, SIGNAL(finished(Hemera::Operation*)), &e, SLOT(quit()));

    if (timeout > 0) {
        QTimer::singleShot(timeout * 1000, &e, SLOT(quit()));
    }

    start();
    e.exec();

    if (!isFinished()) {
        qWarning() << this << tr("timed out while trying to synchronize.");
        return false;
    }

    return !isError();
}

Operation::ExecutionOptions Operation::executionOptions() const
{
    Q_D(const Operation);
    return d->options;
}


}

#include "moc_hemeraoperation.cpp"
