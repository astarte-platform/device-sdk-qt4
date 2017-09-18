#ifndef ASTARTEINTERFACE_H
#define ASTARTEINTERFACE_H

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QSharedDataPointer>

#include <rapidjson/document.h>

#include <astartedevicesdk_global.h>

class AstarteInterfaceData;
class ASTARTEQT4SDKSHARED_EXPORT AstarteInterface
{
public:
    enum Type {
        UnknownType = 0,
        DataStream = 1,
        Properties = 2,
    };

    enum Quality {
        UnknownQuality = 0,
        Producer = 1,
        Consumer = 2,
    };

    AstarteInterface();
    AstarteInterface(const AstarteInterface &other);
    AstarteInterface(const QByteArray &interface, int versionMajor, int versionMinor, Type interfaceType, Quality interfaceQuality);
    ~AstarteInterface();

    static AstarteInterface fromJson(const rapidjson::Document &jsonObj);

    AstarteInterface &operator=(const AstarteInterface &rhs);
    bool operator==(const AstarteInterface &other) const;
    inline bool operator!=(const AstarteInterface &other) const { return !operator==(other); }

    QByteArray interface() const;
    void setInterface(const QByteArray &i);

    int versionMajor() const;
    void setVersionMajor(int v);

    int versionMinor() const;
    void setVersionMinor(int v);

    Type interfaceType() const;
    void setInterfaceType(Type t);

    Quality interfaceQuality() const;
    void setInterfaceQuality(Quality q);

    bool isValid() const;

private:
    QSharedDataPointer<AstarteInterfaceData> d;
};

inline QDataStream &operator>>(QDataStream &s, AstarteInterface &i)
{
    QByteArray interface;
    int versionMajor;
    int versionMinor;
    int interfaceType;
    int interfaceQuality;

    s >> interface >> versionMajor >> versionMinor >> interfaceType >> interfaceQuality;
    i.setInterface(interface);
    i.setVersionMajor(versionMajor);
    i.setVersionMinor(versionMinor);
    i.setInterfaceType(static_cast<AstarteInterface::Type>(interfaceType));
    i.setInterfaceQuality(static_cast<AstarteInterface::Quality>(interfaceQuality));

    return s;
}

inline QDataStream &operator<<(QDataStream &s, const AstarteInterface &i)
{
    return s << i.interface() << i.versionMajor() << i.versionMinor() << static_cast<int>(i.interfaceType()) << static_cast<int>(i.interfaceQuality());
}


#endif // ASTARTEINTERFACE_H
