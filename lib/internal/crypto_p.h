#ifndef ASTARTE_CRYPTO_P_H
#define ASTARTE_CRYPTO_P_H

#include "crypto.h"

#include "utils/hemeraoperation.h"

#include <QtCore/QFutureWatcher>

namespace Astarte {
class Connector;

class ThreadedKeyOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(ThreadedKeyOperation)

public:
    explicit ThreadedKeyOperation(const QString &privateKeyFile, const QString &publicKeyFile, QObject* parent = 0);
    explicit ThreadedKeyOperation(const QString &cn, const QString &privateKeyFile, const QString &publicKeyFile, const QString &csrFile, QObject* parent = 0);
    virtual ~ThreadedKeyOperation();

protected:
    virtual void startImpl();

private Q_SLOTS:
    void onWatcherFinished();
    void onCSRWatcherFinished();

private:
    QString m_cn;
    QString m_privateKeyFile;
    QString m_publicKeyFile;
    QString m_csrFile;

    QFutureWatcher<int> *watcher;
    QFutureWatcher<bool> *csrWatcher;
};
}

#endif // ASTARTE_CRYPTO_P_H
