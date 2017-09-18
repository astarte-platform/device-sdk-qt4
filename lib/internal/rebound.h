#ifndef REBOUND_H
#define REBOUND_H

#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>

#include "wave.h"

class ReboundData;

class Rebound
{
public:
    /// Creates an empty wave with a new unique UUID
    /**
     * @brief Constructs a Rebound from a Wave ID
     *
     * @p waveId The ID of the Wave associated to this Rebound.
     * @p code The response code of this Rebound.
     */
    Rebound(quint64 waveId, ResponseCode code = InvalidCode);
    /**
     * @brief Constructs a Rebound from a Wave
     *
     * @p waveId The Wave associated to this Rebound.
     * @p code The response code of this Rebound.
     */
    Rebound(const Wave &wave, ResponseCode code = InvalidCode);
    Rebound(const Rebound &other);
    ~Rebound();

    Rebound &operator=(const Rebound &rhs);
    bool operator==(const Rebound &other) const;
    inline bool operator!=(const Rebound &other) const { return !operator==(other); }

    /// @returns The rebound's id.
    quint64 id() const;
    void setId(quint64 id);

    /// The Rebound's response code.
    ResponseCode response() const;
    void setResponse(ResponseCode r);

    /// The Rebound's attributes.
    ByteArrayHash attributes() const;
    void setAttributes(const ByteArrayHash &attributes);
    void addAttribute(const QByteArray &attribute, const QByteArray &value);
    bool removeAttribute(const QByteArray &attribute);

    /// The Rebound's payload, if any.
    QByteArray payload() const;
    void setPayload(const QByteArray &p);

    QByteArray serialize() const;
    static Rebound fromBinary(const QByteArray &data);

private:
    QSharedDataPointer<ReboundData> d;
};

#endif // REBOUND_H
