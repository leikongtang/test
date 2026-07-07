#include "mainwindow.h"
#include "installedapp.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CDiskCleaner"));
    app.setApplicationVersion(QStringLiteral("1.3.1"));

    qRegisterMetaType<InstalledApp>("InstalledApp");
    qRegisterMetaType<QVector<InstalledApp>>("QVector<InstalledApp>");

    MainWindow window;
    window.show();

    return app.exec();
}
