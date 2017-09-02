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

    setupContext();

    // New conf to edit
    Q_ASSERT(!m_firewallConfToEdit);
    m_firewallConfToEdit = cloneConf(*m_firewallConf);

    m_engine->load(QUrl("qrc:/qml/main.qml"));
}

bool FortManager::saveConf()
{
    return saveSettings(m_firewallConfToEdit);
}

bool FortManager::applyConf()
{
    Q_ASSERT(m_firewallConfToEdit);
    return saveSettings(cloneConf(*m_firewallConfToEdit));
}

bool FortManager::saveSettings(FirewallConf *newConf)
{
    if (!m_fortSettings->writeConf(*newConf))
        return false;

    m_firewallConf->deleteLater();
    m_firewallConf = newConf;

    return true;
}

void FortManager::handleClosedWindow()
{
    m_engine->deleteLater();
    m_engine = nullptr;

    if (m_firewallConfToEdit && m_firewallConfToEdit != m_firewallConf) {
        m_firewallConfToEdit->deleteLater();
        m_firewallConfToEdit = nullptr;
    }
}

FirewallConf *FortManager::cloneConf(const FirewallConf &conf)
{
    FirewallConf *newConf = new FirewallConf(this);

    const QVariant data = conf.toVariant();
    newConf->fromVariant(data);

    // Flags
    newConf->setFilterEnabled(conf.filterEnabled());
    newConf->ipInclude()->setUseAll(conf.ipInclude()->useAll());
    newConf->ipExclude()->setUseAll(conf.ipExclude()->useAll());
    newConf->setAppBlockAll(conf.appBlockAll());
    newConf->setAppAllowAll(conf.appAllowAll());
    newConf->setAppGroupBits(conf.appGroupBits());

    return newConf;
}
