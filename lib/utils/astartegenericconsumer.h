#ifndef ASTARTE_GENERIC_CONSUMER_H
#define ASTARTE_GENERIC_CONSUMER_H

#include "internal/consumerabstractadaptor.h"

#include "astartedevicesdk.h"

namespace Hyperdrive {
class AstarteTransport;
}

class AstarteGenericConsumer : public ConsumerAbstractAdaptor
{
    Q_OBJECT
    Q_DISABLE_COPY(AstarteGenericConsumer)

public:
    explicit AstarteGenericConsumer(const QByteArray &interface, Astarte::Transport *astarteTransport, AstarteDeviceSDK *parent = 0);
    virtual ~AstarteGenericConsumer();

    void setMappingToTokens(const QHash<QByteArray, QByteArrayList> &mappingToTokens);
    void setMappingToType(const QHash<QByteArray, QVariant::Type> &mappingToType);
    void setMappingToAllowUnset(const QHash<QByteArray, bool> &mappingToAllowUnset);

protected:
    virtual void populateTokensAndStates();
    virtual ConsumerAbstractAdaptor::DispatchResult dispatch(int i, const QByteArray &payload, const QList<QByteArray> &inputTokens);

private:
    inline AstarteDeviceSDK *parent() const { return static_cast<AstarteDeviceSDK *>(QObject::parent()); }

    QHash<QByteArray, QByteArrayList> m_mappingToTokens;
    QHash<QByteArray, QVariant::Type> m_mappingToType;
    QHash<QByteArray, bool> m_mappingToAllowUnset;
};

#endif // ASTARTE_GENERIC_CONSUMER_H
