QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = ZMotionIoReader
VERSION = 1.1.0

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    iopanelwidget.cpp \
    signalconverter.cpp

HEADERS += \
    mainwindow.h \
    iopanelwidget.h \
    signalconverter.h

ZMotionPath = $$PWD/thirdparty/zmotion
INCLUDEPATH += $$ZMotionPath
LIBS += -L$$ZMotionPath -lzauxdll -lzmotion

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CXXFLAGS += /MP
}

win32:CONFIG(release, debug|release) {
    DEPLOYQT = $$[QT_INSTALL_BINS]/windeployqt.exe
    DEPLOY_TARGET = $$shell_path($$OUT_PWD/release/$${TARGET}.exe)
    QMAKE_POST_LINK += $$quote($$DEPLOYQT) --no-translations $$quote($$DEPLOY_TARGET)
    QMAKE_POST_LINK += && $$quote($$QMAKE_COPY) $$quote($$shell_path($$ZMotionPath/zmotion.dll)) $$quote($$shell_path($$OUT_PWD/release/))
    QMAKE_POST_LINK += && $$quote($$QMAKE_COPY) $$quote($$shell_path($$ZMotionPath/zauxdll.dll)) $$quote($$shell_path($$OUT_PWD/release/))
}
