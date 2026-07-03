QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = DragonDeskPet

SOURCES += \
    main.cpp \
    dragonwidget.cpp

HEADERS += \
    dragonwidget.h

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
