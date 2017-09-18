#ifndef _BSON_SERIALIZER_H_
#define _BSON_SERIALIZER_H_

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QVariantHash>
#include <QtCore/QVariantMap>

namespace Util
{

class BSONSerializer
{
    public:
        BSONSerializer();

        QByteArray document() const;

        void appendEndOfDocument();

        void appendDoubleValue(const char *name, double value);
        void appendInt32Value(const char *name, int32_t value);
        void appendInt64Value(const char *name, int64_t value);
        void appendBinaryValue(const char *name, const QByteArray &value);
        void appendASCIIString(const char *name, const QByteArray &string);
        void appendDocument(const char *name, const QByteArray &document);
        void appendString(const char *name, const QString &string);
        void appendDateTime(const char *name, const QDateTime &dateTime);
        void appendBooleanValue(const char *name, bool value);

        void appendValue(const char *name, const QVariant &value);
        void appendDocument(const char *name, const QVariantHash &document);
        void appendDocument(const char *name, const QVariantMap &document);

    private:
        void beginSubdocument(const char *name);
        QByteArray m_doc;
};

}

#endif
