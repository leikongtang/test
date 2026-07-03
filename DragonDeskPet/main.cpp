#include "dragonwidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DragonDeskPet"));
    QApplication::setQuitOnLastWindowClosed(true);

    DragonWidget dragon;
    dragon.show();

    return app.exec();
}
