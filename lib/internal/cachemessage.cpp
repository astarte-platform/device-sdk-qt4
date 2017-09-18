#include "cachemessage.h"

#include <QSharedData>

#include <QtCore/QDebug>

#include "utils/bsondocument.h"
#include "utils/bsonserializer.h"

#include "wave.h"

namespace Astarte {

class CacheMessageData : public QSharedData
{
public:
    CacheMessageData() { }
    CacheMessageData(const CacheMessageData &other)
        : QSharedData(other), target(other.target), interfaceType(other.interfaceType), payload(other.payload), attributes(other.attributes) { }
    ~CacheMessageData() { }

    QByteArray target;
    AstarteInterface::Type interfaceType;
    QByteArray payload;
    QHash<QByteArray, QByteArray> attributes;
};

CacheMessage::CacheMessage()
    : d(new CacheMessageData())
{
}

CacheMessage::CacheMessage(const CacheMessage& other)
    : d(other.d)
{
}

CacheMessage::~CacheMessage()
{
}

CacheMessage& CacheMessage::operator=(const CacheMessage& rhs)
{
    if (this==&rhs) {
        // Protect against self-assignment
        return *this;
    }

    d = rhs.d;
    return *this;
}

bool CacheMessage::operator==(const CacheMessage& other) const
{
    return d->target == other.target() && d->payload == other.payload() && d->interfaceType == other.interfaceType() && d->attributes == other.attributes();
}

QByteArray CacheMessage::payload() const
{
    return d->payload;
}

void CacheMessage::setPayload(const QByteArray& p)
{
    d->payload = p;
}

QByteArray CacheMessage::target() const
{
    return d->target;
}

void CacheMessage::setTarget(const QByteArray& t)
{
    d->target = t;
}

AstarteInterface::Type CacheMessage::interfaceType() const
{
    return d->interfaceType;
}

void CacheMessage::setInterfaceType(AstarteInterface::Type interfaceType)
{
    d->interfaceType = interfaceType;
}

QHash<QByteArray, QByteArray> CacheMessage::attributes() const
{
    return d->attributes;
}

QByteArray CacheMessage::attribute(const QByteArray &attribute) const
{
    return d->attributes.value(attribute);
}

bool CacheMessage::hasAttribute(const QByteArray &attribute) const
{
    return d->attributes.contains(attribute);
}

void CacheMessage::setAttributes(const QHash<QByteArray, QByteArray>& attributes)
{
    d->attributes = attributes;
}

void CacheMessage::addAttribute(const QByteArray& attribute, const QByteArray& value)
{
    d->attributes.insert(attribute, value);
}

bool CacheMessage::removeAttribute(const QByteArray& attribute)
{
    return d->attributes.remove(attribute);
}

QByteArray CacheMessage::takeAttribute(const QByteArray& attribute)
{
    return d->attributes.take(attribute);
}

QByteArray CacheMessage::serialize() const
{
    Util::BSONSerializer s;
    s.appendInt32Value("y", static_cast<int32_t>(Protocol::CacheMessage));
    s.appendASCIIString("t", d->target);
    s.appendBinaryValue("p", d->payload);
    s.appendInt32Value("i", static_cast<int32_t>(d->interfaceType));

    if (!d->attributes.isEmpty()) {
        Util::BSONSerializer sa;
        for (QHash<QByteArray, QByteArray>::const_iterator i = d->attributes.constBegin(); i != d->attributes.constEnd(); ++i) {
            sa.appendASCIIString(i.key(), i.value());
        }
        sa.appendEndOfDocument();
        s.appendDocument("a", sa.document());
    }

    s.appendEndOfDocument();

    return s.document();
}

CacheMessage CacheMessage::fromBinary(const QByteArray &data)
{
    Util::BSONDocument doc(data);
    if (Q_UNLIKELY(!doc.isValid())) {
        qWarning() << "CacheMessage BSON document is not valid!";
        return CacheMessage();
    }
    if (Q_UNLIKELY(doc.int32Value("y") != static_cast<int32_t>(Protocol::CacheMessage))) {
        qWarning() << "Received message is not a CacheMessage";
        return CacheMessage();
    }

    CacheMessage c;
    c.setTarget(doc.byteArrayValue("t"));
    c.setPayload(doc.byteArrayValue("p"));
    c.setInterfaceType(static_cast<AstarteInterface::Type>(doc.int32Value("i")));

    if (doc.contains("a")) {
        Util::BSONDocument attributesDoc = doc.subdocument("a");
        if (!attributesDoc.isValid()) {
            qDebug() << "CacheMessage attributes are not valid\n";
            return CacheMessage();
        }
        c.setAttributes(attributesDoc.byteArrayValuesHash());
    }

    return c;
}

}
