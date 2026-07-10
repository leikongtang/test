#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ZMotionIoReader"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));

    MainWindow window;
    window.show();

    return app.exec();
}
