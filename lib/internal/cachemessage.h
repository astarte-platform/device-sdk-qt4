#ifndef CACHE_MESSAGE_H
#define CACHE_MESSAGE_H

#include "astarteinterface.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QDataStream>

namespace Astarte {

class CacheMessageData;

class CacheMessage {
public:
    CacheMessage();
    CacheMessage(const CacheMessage &other);
    ~CacheMessage();

    CacheMessage &operator=(const CacheMessage &rhs);
    bool operator==(const CacheMessage &other) const;
    inline bool operator!=(const CacheMessage &other) const { return !operator==(other); }

    QByteArray target() const;
    void setTarget(const QByteArray &t);

    AstarteInterface::Type interfaceType() const;
    void setInterfaceType(AstarteInterface::Type interfaceType);

    QByteArray payload() const;
    void setPayload(const QByteArray &p);

    QHash<QByteArray, QByteArray> attributes() const;
    QByteArray attribute(const QByteArray &attribute) const;
    bool hasAttribute(const QByteArray &attribute) const;
    void setAttributes(const QHash<QByteArray, QByteArray> &a);
    void addAttribute(const QByteArray &attribute, const QByteArray &value);
    bool removeAttribute(const QByteArray &attribute);
    QByteArray takeAttribute(const QByteArray &attribute);

    QByteArray serialize() const;
    static CacheMessage fromBinary(const QByteArray &data);

private:
    QSharedDataPointer<CacheMessageData> d;
};

}

#endif // CACHE_MESSAGE_H
