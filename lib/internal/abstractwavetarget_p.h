#ifndef HYPERSPACE_ABSTRACTWAVETARGET_P_H
#define HYPERSPACE_ABSTRACTWAVETARGET_P_H

#include "abstractwavetarget.h"

typedef void (AbstractWaveTarget::* WaveFunction)(quint64 waveId,
                                                  const ByteArrayHash &attributes, const QByteArray &payload);

class AbstractWaveTargetPrivate
{
public:
    AbstractWaveTargetPrivate() {}
    virtual ~AbstractWaveTargetPrivate() {}

    QByteArray interface;
    Astarte::Transport *astarteTransport;
};

#endif
