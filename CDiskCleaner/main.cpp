#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("CDiskCleaner"));
    app.setApplicationVersion(QStringLiteral("1.2.3"));

    MainWindow window;
    window.show();

    return app.exec();
}
