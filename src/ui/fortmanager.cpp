#include "fortmanager.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "conf/addressgroup.h"
#include "conf/appgroup.h"
#include "conf/firewallconf.h"
#include "fortsettings.h"

FortManager::FortManager(QObject *parent) :
    QObject(parent),
    m_fortSettings(new FortSettings(qApp->arguments(), this)),
    m_firewallConf(new FirewallConf(this)),
    m_firewallConfToEdit(nullptr)
{
    m_fortSettings->readConf(*m_firewallConf);

    registerQmlTypes();
}

void FortManager::registerQmlTypes()
{
    qmlRegisterUncreatableType<FortManager>("com.fortfirewall", 1, 0, "FortManager",
                                            "Singleton");
    qmlRegisterUncreatableType<FortSettings>("com.fortfirewall", 1, 0, "FortSettings",
                                             "Singleton");

    qmlRegisterType<AddressGroup>("com.fortfirewall", 1, 0, "AddressGroup");
    qmlRegisterType<AppGroup>("com.fortfirewall", 1, 0, "AppGroup");
    qmlRegisterType<FirewallConf>("com.fortfirewall", 1, 0, "FirewallConf");
}

void FortManager::setupContext()
{
    QQmlContext *context = m_engine->rootContext();

    context->setContextProperty("fortManager", this);
}

void FortManager::showWindow()
{
    m_engine = new QQmlApplicationEngine(this);

    connect(m_engine, &QQmlApplicationEngine::destroyed,
            this, &FortManager::handleClosedWindow);

    createConfToEdit();
    setupContext();

    m_engine->load(QUrl("qrc:/qml/main.qml"));
}

void FortManager::createConfToEdit()
{
    m_firewallConfToEdit = new FirewallConf(this);

    // Clone from current one
    {
        const QVariant data = m_firewallConf->toVariant();
        m_firewallConfToEdit->fromVariant(data);

        m_fortSettings->readConfFlags(*m_firewallConfToEdit);
    }
}

bool FortManager::saveConf()
{
    if (!m_fortSettings->writeConf(*m_firewallConfToEdit))
        return false;

    m_firewallConf->deleteLater();
    m_firewallConf = m_firewallConfToEdit;

    return true;
}

void FortManager::handleClosedWindow()
{
    m_engine->deleteLater();
    m_engine = 0;

    if (m_firewallConfToEdit && m_firewallConfToEdit != m_firewallConf) {
        m_firewallConfToEdit->deleteLater();
        m_firewallConfToEdit = 0;
    }
}
