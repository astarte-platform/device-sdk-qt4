TARGET = astarte-validate-interface

INCLUDEPATH += ../../lib

SOURCES = astarte-validate-interface.cpp

LIBS += -L../../lib/ -lAstarteQt4SDK

macx {
    INCLUDEPATH += /usr/local/Cellar/mosquitto/1.4.14/include
    LIBS += -L/usr/local/Cellar/mosquitto/1.4.14/lib -lmosquittopp
}
unix:!macx {
    LIBS += -lmosquittopp
}
