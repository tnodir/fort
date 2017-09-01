#include "fortmanager.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortsettings.h"

FortManager::FortManager(QObject *parent) :
    QObject(parent),
    m_fortSettings(new FortSettings(qApp->arguments(), this)),
    m_firewallConf(new FirewallConf(this))
{
    m_fortSettings->readConf(*m_firewallConf);
}

void FortManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");
}

void FortManager::setupContext()
{
    QQmlContext *context = m_engine->rootContext();

    context->setContextProperty("fortSettings", m_fortSettings);
    context->setContextProperty("firewallConf", m_firewallConf);
    context->setContextProperty("fortManager", this);
}

void FortManager::showWindow()
{
    m_engine = new QQmlApplicationEngine(this);

    setupContext();

    m_engine->load(QUrl("qrc:/qml/main.qml"));
}

bool FortManager::saveConf()
{
    FirewallConf *newConf = new FirewallConf(this);
    emit fillConf(newConf);

    if (!m_fortSettings->writeConf(*newConf)) {
        delete newConf;
        return false;
    }

    m_firewallConf->deleteLater();
    m_firewallConf = newConf;

    return true;
}
