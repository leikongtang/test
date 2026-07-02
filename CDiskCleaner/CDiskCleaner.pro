QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = CDiskCleaner

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    cleanupscanner.cpp \
    cleanupworker.cpp

HEADERS += \
    mainwindow.h \
    cleanupcategory.h \
    cleanupscanner.h \
    cleanupworker.h

win32 {
    LIBS += -lshell32
}

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
