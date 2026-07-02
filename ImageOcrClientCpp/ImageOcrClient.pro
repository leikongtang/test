QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = ImageOcrClient

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    services/imagehelper.cpp \
    services/ocrclientservice.cpp

HEADERS += \
    mainwindow.h \
    services/imagehelper.h \
    services/ocrclientservice.h

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
