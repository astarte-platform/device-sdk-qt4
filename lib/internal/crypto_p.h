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
