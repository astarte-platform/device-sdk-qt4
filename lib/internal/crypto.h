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
