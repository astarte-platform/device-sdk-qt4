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

#ifndef WAVE_H
#define WAVE_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QUuid>

typedef QHash<QByteArray, QByteArray> ByteArrayHash;

enum ResponseCode
{
    InvalidCode = 0,
    OK = 200,
    Created = 201,
    Accepted = 202,
    PartialInformation = 203,
    NoResponse = 204,
    Moved = 301,
    Method = 303,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    InternalError = 500,
    NotImplemented = 501,
    ServiceUnavailable = 503,
};

namespace Protocol
{

enum Type {
    Unknown = 0,
    ControlType = 1,
    Discovery = 2,
};

enum MessageType
{
    Invalid = 0,
    Wave = 1,
    Rebound = 2,
    Fluctuation = 3,
    Waveguide = 4,
    // Start from 1024 so we don't clash with Hyperspace
    CacheMessage = 1024
};

namespace Control
{
inline quint8 nameExchange() { return 'y'; }
inline quint8 listGatesForHyperdriveInterfaces() { return 'q'; }
inline quint8 listHyperdriveInterfaces() { return 'h'; }
inline quint8 hyperdriveHasInterface() { return 'z'; }
inline quint8 transportsTemplateUrls() { return '8'; }
inline quint8 introspection() { return 'i'; }
inline quint8 cacheMessage() { return 'k'; }
inline quint8 wave() { return 'w'; }
inline quint8 rebound() { return 'r'; }
inline quint8 fluctuation() { return 'f'; }
inline quint8 interfaces() { return 'c'; }
inline quint8 messageTerminator() { return 'T'; }
inline quint8 bigBang() { return 'B'; }
}

inline static QByteArray generateSecret() {
    return QUuid::createUuid().toByteArray();
}

}


class WaveData;
/**
 * @class Wave
 * @ingroup HyperspaceCore
 * @headerfile HyperspaceCore/hyperspaceglobal.h <HyperspaceCore/Global>
 *
 * @brief The base class data structure for Waves.
 *
 * Wave is the base data structure which is used for representing and serializing waves.
 * It encloses every Wave field, and is easily serializable and deserializable through
 * Data Streams.
 *
 * @sa Hyperspace::Rebound
 */
class Wave {
public:
    /// Creates an empty wave with a new unique UUID
    Wave();
    Wave(const Wave &other);
    ~Wave();

    Wave &operator=(const Wave &rhs);
    bool operator==(const Wave &other) const;
    inline bool operator!=(const Wave &other) const { return !operator==(other); }

    /// @returns The wave's id.
    quint64 id() const;
    void setId(quint64 id);

    /// The wave's method, as a string.
    QByteArray method() const;
    void setMethod(const QByteArray &m);

    /// The wave's interface
    QByteArray interface() const;
    void setInterface(const QByteArray &i);

    /// The wave's target, as a full absolute path.
    QByteArray target() const;
    void setTarget(const QByteArray &t);

    /// The wave's attributes.
    ByteArrayHash attributes() const;
    void setAttributes(const ByteArrayHash &attributes);
    void addAttribute(const QByteArray &attribute, const QByteArray &value);
    bool removeAttribute(const QByteArray &attribute);
    QByteArray takeAttribute(const QByteArray &attribute);

    /// The wave's payload.
    QByteArray payload() const;
    void setPayload(const QByteArray &p);

    QByteArray serialize() const;
    static Wave fromBinary(const QByteArray &data);

private:
    Wave(quint64 id);

    QSharedDataPointer<WaveData> d;
};


#endif // WAVE_H
