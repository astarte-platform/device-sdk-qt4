TARGET = AstarteQt4SDK
TEMPLATE = lib
CONFIG += sharedlib
QT    += network sql
QT    -= gui

DEFINES += QT4SDK_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += ../json/

PKGCONFIG += openssl

macx {
    INCLUDEPATH += /usr/local/Cellar/openssl/1.0.2l/include
    LIBS += -L/usr/local/Cellar/openssl/1.0.2l/lib -lcrypto -lssl -L/usr/local/Cellar/mosquitto/1.4.14/lib -lmosquittopp
}
unix:!macx {
    LIBS += -lmosquittopp
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    astarteinterface.cpp \
    utils/bsondocument.cpp \
    utils/bsonserializer.cpp \
    internal/crypto.cpp \
    utils/hemeraoperation.cpp \
    utils/hemeracommonoperations.cpp \
    internal/verifycertificateoperation.cpp \
    internal/endpoint.cpp \
    internal/httpendpoint.cpp \
    internal/mqttclientwrapper.cpp \
    internal/transport.cpp \
    internal/transportcache.cpp \
    utils/hemeraasyncinitobject.cpp \
    internal/cachemessage.cpp \
    internal/wave.cpp \
    internal/rebound.cpp \
    internal/fluctuation.cpp \
    internal/abstractwavetarget.cpp \
    internal/producerabstractinterface.cpp \
    utils/transportdatabasemanager.cpp \
    internal/consumerabstractadaptor.cpp \
    utils/astartegenericconsumer.cpp \
    utils/astartegenericproducer.cpp \
    utils/validateinterfaceoperation.cpp \
    astartedevicesdk.cpp

HEADERS += \
    astarteinterface.h \
    utils/bsondocument.h \
    utils/apple_endian.h \
    utils/bsonserializer.h \
    internal/crypto.h \
    utils/hemeraoperation.h \
    utils/hemeraoperation_p.h \
    internal/crypto_p.h \
    utils/hemeracommonoperations.h \
    internal/verifycertificateoperation.h \
    internal/endpoint.h \
    internal/httpendpoint.h \
    internal/endpoint_p.h \
    utils/utils.h \
    internal/mqttclientwrapper.h \
    internal/httpendpoint_p.h \
    internal/transport.h \
    internal/transportcache.h \
    utils/hemeraasyncinitobject.h \
    utils/hemeraasyncinitobject_p.h \
    internal/cachemessage.h \
    internal/wave.h \
    internal/rebound.h \
    internal/fluctuation.h \
    internal/abstractwavetarget.h \
    internal/abstractwavetarget_p.h \
    internal/producerabstractinterface.h \
    utils/transportdatabasemanager.h \
    internal/consumerabstractadaptor.h \
    utils/astartegenericconsumer.h \
    utils/astartegenericproducer.h \
    utils/validateinterfaceoperation.h \
    astartedevicesdk.h \
    astartedevicesdk_p.h \
    astartedevicesdk_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES +=

interface_schema.path = /usr/share/astarte-sdk
interface_schema.files = interface.json

db_migrations.path = /usr/share/astarte-sdk/db/migrations
db_migrations.files = db/migrations/*

header_files.files = \
    astartedevicesdk.h \
    astartedevicesdk_global.h
header_utils_files.files = \
    utils/hemeraasyncinitobject.h \
    utils/hemeraoperation.h

header_files.path = /usr/include/astarteqt4sdk
header_utils_files.path = /usr/include/astarteqt4sdk/utils

INSTALLS += interface_schema db_migrations header_files header_utils_files

