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
