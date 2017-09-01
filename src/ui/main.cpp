#include <QGuiApplication>
#include <QQmlEngine>

#include "conf/firewallconf.h"
#include "fortsettings.h"
#include "fortwindow.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    FirewallConf firewallConf;
    FortSettings fortSettings(qApp->arguments());
    fortSettings.readConf(firewallConf);

    FortWindow fortWindow(&firewallConf, &fortSettings);
    fortWindow.show();

    return app.exec();
}
