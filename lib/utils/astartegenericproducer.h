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

#ifndef ASTARTE_GENERIC_PRODUCER_H
#define ASTARTE_GENERIC_PRODUCER_H

#include "internal/producerabstractinterface.h"

#include "astarteinterface.h"
#include "astartedevicesdk.h"

namespace Astarte {
class Transport;
}

class AstarteGenericProducer : public ProducerAbstractInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AstarteGenericProducer)

public:
    explicit AstarteGenericProducer(const QByteArray &interface, AstarteInterface::Type interfaceType,
                                    Astarte::Transport *astarteTransport, QObject *parent = 0);
    virtual ~AstarteGenericProducer();

    bool sendData(const QVariant &value, const QByteArray &target, const QDateTime &timestamp,
                  const QVariantHash &metadata);
    bool sendData(const QVariantHash &value, const QByteArray &target, const QDateTime &timestamp,
                  const QVariantHash &metadata);

    void setMappingToTokens(const QHash<QByteArray, QByteArrayList> &mappingToTokens);
    void setMappingToType(const QHash<QByteArray, QVariant::Type> &mappingToType);
    void setMappingToRetention(const QHash<QByteArray, Retention> &m_mappingToRetention);
    void setMappingToReliability(const QHash<QByteArray, Reliability> &m_mappingToReliability);
    void setMappingToExpiry(const QHash<QByteArray, int> &m_mappingToExpiry);

    QByteArrayList mappings() const;
    QHash<QByteArray, QVariant::Type> mappingToType() const;
    QHash<QByteArray, QByteArrayList> mappingToTokens() const;

protected:
    virtual void populateTokensAndStates();
    virtual ProducerAbstractInterface::DispatchResult dispatch(int i, const QByteArray &payload, const QList<QByteArray> &inputTokens);

private:
    QHash<QByteArray, QByteArrayList> m_mappingToTokens;
    QHash<QByteArray, QVariant::Type> m_mappingToType;
    QHash<QByteArray, Retention> m_mappingToRetention;
    QHash<QByteArray, Reliability> m_mappingToReliability;
    QHash<QByteArray, int> m_mappingToExpiry;

    AstarteInterface::Type m_interfaceType;
};

#endif // ASTARTE_GENERIC_PRODUCER_H
