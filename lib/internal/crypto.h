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

#ifndef ASTARTE_CRYPTO_H
#define ASTARTE_CRYPTO_H

#include "utils/hemeraasyncinitobject.h"
#include <QtCore/QFlags>

#include "utils/hemeraoperation.h"

namespace Astarte {

// Define some convenience paths here.

class Crypto : public Hemera::AsyncInitObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Crypto)

public:
    enum AuthenticationDomain {
        NoAuthenticationDomain = 0,
        DeviceAuthenticationDomain = 1 << 0,
        CustomerAuthenticationDomain = 1 << 1,
        AnyAuthenticationDomain = 255
    };
    Q_ENUMS(AuthenticationDomain)
    Q_DECLARE_FLAGS(AuthenticationDomains, AuthenticationDomain)

    static Crypto * instance();

    virtual ~Crypto();

    bool isKeyStoreAvailable() const;
    Hemera::Operation *generateAstarteKeyStore(bool forceGeneration = false);

    QByteArray sign(const QByteArray &payload, AuthenticationDomains = AnyAuthenticationDomain);

    static QString cryptoBasePath();
    static QString pathToCertificateRequest();
    static QString pathToPrivateKey();
    static QString pathToPublicKey();

    static void setCryptoBasePath(const QString &basePath);
    static void setHardwareId(const QByteArray &hardwareId);

Q_SIGNALS:
    void keyStoreAvailabilityChanged();

protected Q_SLOTS:
    virtual void initImpl();

private:
    explicit Crypto(QObject *parent = 0);

    class Private;
    Private * const d;

    static QString s_cryptoBasePath;

    friend class ThreadedKeyOperation;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Astarte::Crypto::AuthenticationDomains)

#endif // ASTARTE_CRYPTO_H
