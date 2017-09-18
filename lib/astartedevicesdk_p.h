#ifndef QT4SDK_P_H
#define QT4SDK_P_H

#include "astartedevicesdk.h"

#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"

#include "astarteinterface.h"

#include "internal/producerabstractinterface.h"

using namespace rapidjson;

class AstarteDeviceSDK::Private {
public:
    Private(AstarteDeviceSDK *q) : q(q) {}
    ~Private() {}

    AstarteDeviceSDK * const q;

    Astarte::Transport *astarteTransport;

    QString configurationPath;
    QString interfacesDir;

    QByteArray hardwareId;

    QHash<QByteArray, AstarteGenericProducer *> producers;
    QHash<QByteArray, AstarteGenericConsumer *> consumers;

    void loadInterfaces();

    void createProducer(const AstarteInterface &interface, const rapidjson::Document &producerObject);
    void createConsumer(const AstarteInterface &interface, const rapidjson::Document &consumerObject);

    QVariant::Type typeStringToVariantType(const QString &typeString) const;
    Retention retentionStringToRetention(const QString &retentionString) const;
    Reliability reliabilityStringToReliability(const QString &reliabilityString) const;

    void receiveValue(const QByteArray &interface, const QByteArray &path, const QVariant &value);
    void unsetValue(const QByteArray &interface, const QByteArray &path);
};

#endif // QT4SDK_P_H
