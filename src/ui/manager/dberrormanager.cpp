#include "dberrormanager.h"

#include <QLoggingCategory>
#include <QTimer>

#include <sqlite/sqlitedb.h>

#include <fortmanager.h>
#include <fortsettings.h>
#include <util/ioc/ioccontainer.h>

namespace {

const QLoggingCategory LC("manager.dbError");

void sqliteLogHandler(void * /*context*/, int errCode, const char *message)
{
    const auto messageLine =
            QString("%1: %2").arg(QString::number(errCode), qUtf8Printable(message));

    if (SqliteDb::isDebugError(errCode)) {
        qCDebug(LC) << messageLine;
    } else if (SqliteDb::isIoError(errCode)) {
        qCCritical(LC) << messageLine;
    } else {
        qCWarning(LC) << messageLine;
    }
}

}

DbErrorManager::DbErrorManager(QObject *parent) : QObject(parent) { }

void DbErrorManager::setUp()
{
    // SQLite Log Callback
    SqliteDb::setErrorLogCallback(sqliteLogHandler, /*context=*/this);

    setupTimer();
}

void DbErrorManager::checkConfDir()
{
    if (m_confDir.checkIsValid())
        return;

    if (m_confDir.open()) {
        // Restart on profile's drive mounted
        IoC<FortManager>()->processRestartRequired(tr("Profile's drive mounted"));
    }
}

void DbErrorManager::setupTimer()
{
    auto timer = new QTimer(this);
    timer->setInterval(1500);
    timer->start();

    connect(timer, &QTimer::timeout, this, &DbErrorManager::checkConfDir);

    setupDirInfo();
}

void DbErrorManager::setupDirInfo()
{
    auto settings = IoC<FortSettings>();

    m_confDir.setPath(settings->confFilePath());
    m_confDir.open();
}
