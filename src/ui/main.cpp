#include <QGuiApplication>

#include "fortmanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    FortManager fortManager;
    fortManager.showWindow();

    return app.exec();
}
