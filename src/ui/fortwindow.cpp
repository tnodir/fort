#include "fortwindow.h"

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortsettings.h"

FortWindow::FortWindow(FirewallConf *firewallConf,
                       FortSettings *fortSettings,
                       QObject *parent) :
    QObject(parent),
    m_firewallConf(firewallConf),
    m_fortSettings(fortSettings)
{
}

void FortWindow::registerQmlTypes()
{
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");
}

void FortWindow::setupContext()
{
    QQmlContext *context = m_engine->rootContext();

    context->setContextProperty("firewallConf", m_firewallConf);
    context->setContextProperty("fortSettings", m_fortSettings);
    context->setContextProperty("fortWindow", this);
}

void FortWindow::show()
{
    m_engine = new QQmlApplicationEngine(this);

    setupContext();

    m_engine->load(QUrl("qrc:/qml/main.qml"));
}

bool FortWindow::save()
{
    return m_fortSettings->writeConf(*m_firewallConf);
}
