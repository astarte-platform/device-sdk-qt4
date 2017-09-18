#-------------------------------------------------
#
# Project created by QtCreator 2017-09-11T11:59:54
#
#-------------------------------------------------

TEMPLATE = subdirs

DEFINES += QT4SDK_LIBRARY

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

SUBDIRS = lib astarte-validate-interface

astarte-validate-interface.subdir = tools/astarte-validate-interface
astarte-validate-interface.depends = lib
