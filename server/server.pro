QT += core network sql charts widgets
QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = ExamServer
TEMPLATE = app

# 包含公共模块
include(../common/common.pri)

SOURCES += \
    main.cpp \
    ExamServer.cpp \
    ExamTcpServer.cpp \
    RequestHandler.cpp

HEADERS += \
    ExamServer.h \
    ExamTcpServer.h \
    RequestHandler.h

# 目标输出目录
DESTDIR = ../build/server

# 数据库文件
RESOURCES += \
    ../resources/resources.qrc

# Windows 下使用原生 Winsock API 需要链接 ws2_32
win32: LIBS += -lws2_32
