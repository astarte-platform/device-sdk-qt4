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

#ifndef _HYPERSPACEBASEADAPTOR_H_
#define _HYPERSPACEBASEADAPTOR_H_

#include <QtCore/QObject>
#include <QtCore/QByteArray>

#include "abstractwavetarget.h"

class QDateTime;

namespace Astarte {
class Transport;
}

class ConsumerAbstractAdaptor : public AbstractWaveTarget
{
    Q_OBJECT

    public:
        enum DispatchResult {
            Success,
            IndexNotFound,
            CouldNotConvertPayload
        };

        explicit ConsumerAbstractAdaptor(const QByteArray &interface, Astarte::Transport *astarteTransport, QObject *parent);
        virtual ~ConsumerAbstractAdaptor();

    protected:
        virtual void waveFunction(const Wave &wave);

        void insertTransition(int state, QByteArray token, int newState);
        void insertDispatchState(int state, int dispatchIndex);

        int dispatchIndex(const QList<QByteArray> &inputTokens);

        virtual void populateTokensAndStates() = 0;
        virtual DispatchResult dispatch(int i, const QByteArray &value, const QList<QByteArray> &inputTokens) = 0;

        bool payloadToValue(const QByteArray &payload, QByteArray *value);
        bool payloadToValue(const QByteArray &payload, int *value);
        bool payloadToValue(const QByteArray &payload, qint64 *value);
        bool payloadToValue(const QByteArray &payload, bool *value);
        bool payloadToValue(const QByteArray &payload, double *value);
        bool payloadToValue(const QByteArray &payload, QString *value);
        bool payloadToValue(const QByteArray &payload, QDateTime *value);

    private:
        class Private;
        Private *const d;
};

#endif
