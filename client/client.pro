QT += core gui network sql charts widgets

CONFIG += c++11

TARGET = OnlineExamClient
TEMPLATE = app

# 包含公共模块
include(../common/common.pri)

# 客户端源文件
SOURCES += \
    main.cpp \
    LoginDialog.cpp \
    ExamWindow.cpp \
    QuestionWidget.cpp \
    ExamResultDialog.cpp \
    StudentMainWindow.cpp

# 客户端头文件
HEADERS += \
    LoginDialog.h \
    ExamWindow.h \
    QuestionWidget.h \
    ExamResultDialog.h \
    StudentMainWindow.h

# 管理端源文件
SOURCES += \
    AdminMainWindow.cpp \
    QuestionManager.cpp \
    ScoreStatistics.cpp \
    UserManager.cpp

# 管理端头文件
HEADERS += \
    AdminMainWindow.h \
    QuestionManager.h \
    ScoreStatistics.h \
    UserManager.h

# 目标输出目录
DESTDIR = ../build/client

# 资源文件
RESOURCES += \
    ../resources/resources.qrc
