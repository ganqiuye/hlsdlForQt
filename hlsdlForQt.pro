#-------------------------------------------------
#
# Project created by QtCreator 2021-03-10T11:27:15
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hlsdlForQt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        hlsdl.cpp \
    core/curl.c \
    core/hls.c \
    core/misc.c \
    core/mpegts.c \
    core/msg.c \
    core/aes.c \
    core/aes_openssl.c \
    downloadthread.cpp \
    qualitydlg.cpp

HEADERS  += hlsdl.h \
    core/curl.h \
    core/hls.h \
    core/misc.h \
    core/mpegts.h \
    core/msg.h \
    core/aes.h \
    downloadthread.h \
    qualitydlg.h

INCLUDEPATH += ./libcurl \
               ./libopenssl/include

LIBS += -L$$PWD/libopenssl/lib32 -llibssl -llibcrypto \
        -L$$PWD/libcurl -llibcurl

FORMS    += hlsdl.ui
