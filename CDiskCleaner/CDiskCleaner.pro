QT       += core gui concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = CDiskCleaner

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    cleanupscanner.cpp \
    cleanupworker.cpp \
    appuninstaller.cpp \
    appuninstallworker.cpp \
    applistmodel.cpp

HEADERS += \
    mainwindow.h \
    cleanupcategory.h \
    cleanupscanner.h \
    cleanupworker.h \
    installedapp.h \
    appuninstaller.h \
    appuninstallworker.h \
    applistmodel.h

win32 {
    LIBS += -lshell32 -ladvapi32
}

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

win32:CONFIG(release, debug|release) {
    DEPLOYQT = $$[QT_INSTALL_BINS]/windeployqt.exe
    DEPLOY_TARGET = $$shell_path($$OUT_PWD/release/$${TARGET}.exe)
    QMAKE_POST_LINK += $$quote($$DEPLOYQT) --no-translations $$quote($$DEPLOY_TARGET)
}
