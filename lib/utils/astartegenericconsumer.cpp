#include "astartegenericconsumer.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "internal/transport.h"

#include "astartedevicesdk_p.h"

AstarteGenericConsumer::AstarteGenericConsumer(const QByteArray &interface, Astarte::Transport *astarteTransport, AstarteDeviceSDK *parent)
    : ConsumerAbstractAdaptor(interface, astarteTransport, parent)
{
}

AstarteGenericConsumer::~AstarteGenericConsumer()
{
}

void AstarteGenericConsumer::setMappingToTokens(const QHash<QByteArray, QByteArrayList> &mappingToTokens)
{
    m_mappingToTokens = mappingToTokens;
}

void AstarteGenericConsumer::setMappingToType(const QHash<QByteArray, QVariant::Type> &mappingToType)
{
    m_mappingToType = mappingToType;
}

void AstarteGenericConsumer::setMappingToAllowUnset(const QHash<QByteArray, bool> &mappingToAllowUnset)
{
    m_mappingToAllowUnset = mappingToAllowUnset;
}

void AstarteGenericConsumer::populateTokensAndStates()
{
}

ConsumerAbstractAdaptor::DispatchResult AstarteGenericConsumer::dispatch(int i, const QByteArray &payload, const QList<QByteArray> &inputTokens)
{
    QHash<QByteArray, QByteArrayList>::const_iterator it;

    for (it = m_mappingToTokens.constBegin(); it != m_mappingToTokens.constEnd(); it++) {
        if (inputTokens.size() != it.value().size()) {
            continue;
        }

        bool mappingMatch = true;
        for (int itt = 0; itt < inputTokens.size(); ++itt) {
            if (inputTokens.at(itt).startsWith("%{")) {
                continue;
            }

            if (inputTokens.at(itt) != it.value().at(itt)) {
                mappingMatch = false;
            }
        }

        if (!mappingMatch) {
            continue;
        }

        QByteArray matchedMapping = it.key();

        QByteArray joinedInputTokens;
        Q_FOREACH (const QByteArray &inputToken, inputTokens) {
            joinedInputTokens += inputToken;
            joinedInputTokens += '/';
        }

        QByteArray path = QByteArray("/%1").replace("%1", joinedInputTokens);

        if (payload.isEmpty()) {
            if (m_mappingToAllowUnset.value(matchedMapping)) {
                parent()->d->unsetValue(interface(), path);
                return Success;
            } else {
                return CouldNotConvertPayload;
            }
        }

        switch (m_mappingToType.value(matchedMapping)) {
            case QMetaType::Bool: {
                bool value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::Int: {
                int value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::LongLong: {
                qint64 value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::QByteArray: {
                QByteArray value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::Double: {
                double value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::QString: {
                QString value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            case QMetaType::QDateTime: {
                QDateTime value;
                if (!payloadToValue(payload, &value)) return CouldNotConvertPayload;
                parent()->d->receiveValue(interface(), path, value);
                return Success;
            }
            default:
                return CouldNotConvertPayload;
        }
    }

    // If we're here, mapping not found
    return IndexNotFound;
}
