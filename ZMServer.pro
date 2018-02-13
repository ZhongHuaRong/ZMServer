TEMPLATE = app
CONFIG += console
CONFIG +=c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    TcpServer.cpp \
    DBManagement.cpp \
    CThread.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    TcpServer.h \
    DBManagement.h \
    CThread.h

LIBS += -lpthread -lmysqlclient
