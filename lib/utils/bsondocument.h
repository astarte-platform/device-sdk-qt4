#ifndef _HYPERSPACE_BSONDOCUMENT_H_
#define _HYPERSPACE_BSONDOCUMENT_H_

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QVariant>

namespace Util
{

class BSONDocument
{
    public:
        BSONDocument(const QByteArray &document);
        int size() const;
        bool isValid() const;
        bool contains(const char *name) const;

        QVariant value(const char *name, QVariant defaultValue = QVariant()) const;

        double doubleValue(const char *name, double defaultValue = 0.0) const;
        QByteArray byteArrayValue(const char *name, const QByteArray &defaultValue = QByteArray()) const;
        QString stringValue(const char *name, const QString &defaultValue = QString()) const;
        QDateTime dateTimeValue(const char *name, const QDateTime &defaultValue = QDateTime()) const;
        int32_t int32Value(const char *name, int32_t defaultValue = 0) const;
        int64_t int64Value(const char *name, int64_t defaultValue = 0) const;
        bool booleanValue(const char *name, bool defaultValue = false) const;

        BSONDocument subdocument(const char *name) const;
        QHash<QByteArray, QByteArray> byteArrayValuesHash() const;

        QByteArray toByteArray() const;

    private:
        const QByteArray m_doc;
};

} // Util

#endif
