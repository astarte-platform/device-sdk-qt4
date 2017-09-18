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
