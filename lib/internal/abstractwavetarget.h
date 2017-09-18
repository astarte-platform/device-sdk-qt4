/*
 *
 */

#ifndef ABSTRACTWAVETARGET_H
#define ABSTRACTWAVETARGET_H

#include <QtCore/QObject>

#include "fluctuation.h"
#include "rebound.h"

namespace Astarte {
class Transport;
}

/**
 * @defgroup HyperspaceCore Hyperspace Core
 *
 * Hyperspace Core contains all the basic types and mechanism to enable Hyperspace usage, both as a server and as a client,
 * for Hemera applications.
 *
 * It is contained in the Hyperspace:: namespace.
 */

class AbstractWaveTargetPrivate;

class AbstractWaveTarget : public QObject
{
    Q_OBJECT

    Q_DECLARE_PRIVATE_D(d_w_ptr, AbstractWaveTarget)
    Q_DISABLE_COPY(AbstractWaveTarget)

public:
    AbstractWaveTarget(const QByteArray &interface, Astarte::Transport *astarteTransport, QObject *parent = 0);
    virtual ~AbstractWaveTarget();

    QByteArray interface() const;

    bool isReady() const;

Q_SIGNALS:
    void ready();

protected Q_SLOTS:
    /**
     * @brief Send a rebound for a received wave
     *
     * Call this method upon processing a wave. The rebound must have the same ID of its corresponding Wave.
     */
    void sendRebound(const Rebound &rebound);

    /**
     * @brief Send a fluctuation for this target
     *
     * Call this method whenever the internal representation (e.g.: GET) of the target changes.
     */
    void sendFluctuation(const QByteArray &targetPath, const Fluctuation &payload);

    void handleReceivedWave(const QByteArray &interface, const Wave &wave);


protected:
    AbstractWaveTargetPrivate * const d_w_ptr;

    virtual void waveFunction(const Wave &wave) = 0;
};

#endif // ABSTRACTWAVE_H
