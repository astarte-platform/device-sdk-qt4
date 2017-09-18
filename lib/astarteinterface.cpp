#include "astarteinterface.h"

#include <QtCore/QDebug>
#include <QtCore/QSharedData>

class AstarteInterfaceData : public QSharedData
{
public:
    AstarteInterfaceData() : interfaceType(AstarteInterface::UnknownType), interfaceQuality(AstarteInterface::UnknownQuality) { }
    AstarteInterfaceData(const QByteArray &interface, int versionMajor, int versionMinor, AstarteInterface::Type interfaceType, AstarteInterface::Quality interfaceQuality)
        : interface(interface), versionMajor(versionMajor), versionMinor(versionMinor), interfaceType(interfaceType)
        , interfaceQuality(interfaceQuality) { }
    AstarteInterfaceData(const AstarteInterfaceData &other)
        : QSharedData(other), interface(other.interface), versionMajor(other.versionMajor), versionMinor(other.versionMinor)
        , interfaceType(other.interfaceType), interfaceQuality(other.interfaceQuality) { }
    ~AstarteInterfaceData() { }

    QByteArray interface;
    int versionMajor;
    int versionMinor;
    AstarteInterface::Type interfaceType;
    AstarteInterface::Quality interfaceQuality;
};

AstarteInterface::AstarteInterface()
    : d(new AstarteInterfaceData())
{
}

AstarteInterface::AstarteInterface(const AstarteInterface &other)
    : d(other.d)
{
}

AstarteInterface::AstarteInterface(const QByteArray &interface, int versionMajor, int versionMinor, Type interfaceType, Quality interfaceQuality)
    : d(new AstarteInterfaceData(interface, versionMajor, versionMinor, interfaceType, interfaceQuality))
{
}

AstarteInterface::~AstarteInterface()
{
}

AstarteInterface& AstarteInterface::operator=(const AstarteInterface &rhs)
{
    if (this == &rhs) {
        // Protect against self-assignment
        return *this;
    }

    d = rhs.d;
    return *this;
}

bool AstarteInterface::operator==(const AstarteInterface &other) const
{
    return d->interface == other.interface() && d->versionMajor == other.versionMajor() && d->versionMinor == other.versionMinor()
           && d->interfaceType == other.interfaceType() && d->interfaceQuality == other.interfaceQuality();
}

QByteArray AstarteInterface::interface() const
{
    return d->interface;
}

void AstarteInterface::setInterface(const QByteArray &i)
{
    d->interface = i;
}

int AstarteInterface::versionMajor() const
{
    return d->versionMajor;
}

void AstarteInterface::setVersionMajor(int v)
{
    d->versionMajor = v;
}

int AstarteInterface::versionMinor() const
{
    return d->versionMinor;
}

void AstarteInterface::setVersionMinor(int v)
{
    d->versionMinor = v;
}

AstarteInterface::Type AstarteInterface::interfaceType() const
{
    return d->interfaceType;
}

void AstarteInterface::setInterfaceType(Type t)
{
    d->interfaceType = t;
}

AstarteInterface::Quality AstarteInterface::interfaceQuality() const
{
    return d->interfaceQuality;
}

void AstarteInterface::setInterfaceQuality(Quality q)
{
    d->interfaceQuality = q;
}

bool AstarteInterface::isValid() const
{
    return !d->interface.isEmpty();
}

AstarteInterface AstarteInterface::fromJson(const rapidjson::Document &jsonObj)
{
    QByteArray interface;
    if (jsonObj.HasMember("interface_name")) {
        interface = jsonObj["interface_name"].GetString();
    } else if (jsonObj.HasMember("interface")) {
        interface = jsonObj["interface"].GetString();
        qWarning() << "interface is deprecated, use interface_name";
    } else {
        qWarning() << "interface_name missing in AstarteInterface JSON object";
        return AstarteInterface();
    }

    int versionMajor;
    if (jsonObj.HasMember("version_major")) {
        versionMajor = jsonObj["version_major"].GetInt();
    } else {
        qWarning() << "version_major missing in AstarteInterface JSON object for interface " << interface;
        return AstarteInterface();
    }

    int versionMinor;
    if (jsonObj.HasMember("version_minor")) {
        versionMinor = jsonObj["version_minor"].GetInt();
    } else {
        qWarning() << "version_minor missing in AstarteInterface JSON object for interface " << interface;
        return AstarteInterface();
    }

    Type interfaceType;
    if (jsonObj.HasMember("type")) {
        QString typeString = QLatin1String(jsonObj["type"].GetString());
        if (typeString == "datastream") {
            interfaceType = AstarteInterface::DataStream;
        } else if (typeString == "properties") {
            interfaceType = AstarteInterface::Properties;
        } else {
            qWarning() << "Invalid type in AstarteInterface JSON object for interface " << interface;
            return AstarteInterface();
        }
    } else {
        qWarning() << "type missing in AstarteInterface JSON object for interface " << interface;
        return AstarteInterface();
    }

    Quality interfaceQuality;
    if (jsonObj.HasMember("quality")) {
        QString qualityString = QLatin1String(jsonObj["quality"].GetString());
        if (qualityString == "producer") {
            interfaceQuality = AstarteInterface::Producer;
        } else if (qualityString == "consumer") {
            interfaceQuality = AstarteInterface::Consumer;
        } else {
            qWarning() << "Invalid quality in AstarteInterface JSON object for interface " << interface;
            return AstarteInterface();
        }
    } else {
        qWarning() << "quality missing in AstarteInterface JSON object for interface " << interface;
        return AstarteInterface();
    }

    return AstarteInterface(interface, versionMajor, versionMinor, interfaceType, interfaceQuality);
}
