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

CONFIG(release, debug|release):win32 {
    DEPLOYQT = $$shell_path($$[QT_INSTALL_BINS]/windeployqt.exe)
    DEPLOYEXE = $$shell_path($$OUT_PWD/release/$$TARGET.exe)
    QMAKE_POST_LINK += $$quote($$DEPLOYQT) --release --no-translations $$quote($$DEPLOYEXE)
}

win32:CONFIG(release, debug|release) {
    DEPLOYQT = $$[QT_INSTALL_BINS]/windeployqt.exe
    DEPLOY_TARGET = $$shell_path($$OUT_PWD/release/$${TARGET}.exe)
    QMAKE_POST_LINK += $$quote($$DEPLOYQT) --no-translations $$quote($$DEPLOY_TARGET)
}
