TEMPLATE = app
CONFIG += console
CONFIG +=c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    TcpServer.cpp \
    DBManagement.cpp \
    CThread.cpp \
    SMTP.cpp \
    HttpReq.cpp \
    md5.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    TcpServer.h \
    DBManagement.h \
    CThread.h \
    SMTP.h \
    HttpReq.h \
    md5.h

LIBS += -lpthread -lmysqlclient
