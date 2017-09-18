#ifndef UTILS_H
#define UTILS_H

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QHash>

#include "astartedevicesdk.h"

namespace Utils {

inline void seedRNG()
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
}

inline qreal randomizedInterval(qreal minimumInterval, qreal delayCoefficient)
{
    return minimumInterval + (minimumInterval * delayCoefficient * ((qreal)qrand() / RAND_MAX));
}

inline int majorVersion() { return 0; }
inline int minorVersion() { return 8; }
inline int releaseVersion() { return 0; }

inline bool verifyPathMatch(const QByteArrayList path1, const QByteArrayList path2) {
    if (path1.size() != path2.size()) {
        return false;
    }

    for (int i = 0; i < path1.size(); ++i) {
        if (path1.at(i).startsWith("%{")) {
            continue;
        }

        if (path1.at(i) != path2.at(i)) {
            return false;
        }
    }

    return true;
}

inline bool verifySplitTokenMatch(const QHash<QByteArray, QByteArrayList> &token2, const QHash<QByteArray, QByteArrayList> &token1) {
    for (QHash<QByteArray, QByteArrayList>::const_iterator it = token1.constBegin(); it != token1.constEnd(); it++) {
        bool matched = false;
        for (QHash<QByteArray, QByteArrayList>::const_iterator yt = token2.constBegin(); yt != token2.constEnd(); yt++) {
            if (!Utils::verifyPathMatch(it.value(), yt.value())) {
                continue;
            }
            matched = true;
            break;
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

} // Utils


#endif // UTILS_H
