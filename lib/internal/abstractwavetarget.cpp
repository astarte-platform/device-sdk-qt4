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

#include "abstractwavetarget_p.h"

#include <QtCore/QDebug>
#include <QtCore/QSharedData>
#include <QtCore/QTimer>

#include "transport.h"

#include "cachemessage.h"

#include "wave.h"

AbstractWaveTarget::AbstractWaveTarget(const QByteArray &interface, Astarte::Transport *astarteTransport, QObject *parent)
    : QObject(parent)
    , d_w_ptr(new AbstractWaveTargetPrivate)
{
    Q_D(AbstractWaveTarget);
    d->interface = interface;
    d->astarteTransport = astarteTransport;

    connect(d->astarteTransport, SIGNAL(waveReceived(QByteArray,Wave)), this, SLOT(handleReceivedWave(QByteArray,Wave)));
}

AbstractWaveTarget::~AbstractWaveTarget()
{
    delete d_w_ptr;
}

QByteArray AbstractWaveTarget::interface() const
{
    Q_D(const AbstractWaveTarget);

    return d->interface;
}

bool AbstractWaveTarget::isReady() const
{
    return true;
}

void AbstractWaveTarget::handleReceivedWave(const QByteArray &interface, const Wave &wave)
{
    Q_D(AbstractWaveTarget);
    if (interface != d->interface) {
        return;
    }

    // TODO: Make async?
    waveFunction(wave);
}

void AbstractWaveTarget::sendRebound(const Rebound &rebound)
{
    Q_UNUSED(rebound)
}

void AbstractWaveTarget::sendFluctuation(const QByteArray &targetPath, const Fluctuation &payload)
{
    Astarte::CacheMessage c;
    c.setTarget(QByteArray("/%1%2").replace("%1", interface()).replace("%2", targetPath));
    c.setPayload(payload.payload());
    c.setAttributes(payload.attributes());
    AstarteInterface::Type interfaceType =
        static_cast<AstarteInterface::Type>(payload.attributes().value("interfaceType").toInt());
    c.setInterfaceType(interfaceType);
    // TODO: Make this async?
    Q_D(AbstractWaveTarget);
    d->astarteTransport->cacheMessage(c);
}

#include "moc_abstractwavetarget.cpp"
