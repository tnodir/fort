#include "dberrormanager.h"

#include <QLoggingCategory>
#include <QTimer>

#include <sqlite/sqlitedb.h>

#include <fortglobal.h>
#include <fortmanager.h>
#include <fortsettings.h>

using namespace Fort;

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

void DbErrorManager::checkProfileDir()
{
    if (m_confDir.checkIsValid())
        return;

    qCDebug(LC) << "Profile drive is offline";

    if (m_confDir.open()) {
        // Restart on profile's drive mounted
        fortManager()->processRestartRequired(tr("Profile's drive mounted"));
    }
}

void DbErrorManager::setupTimer()
{
    auto settings = Fort::settings();

    const bool checkProfileOnline = (settings->checkProfileOnline() || settings->isPortable());
    if (!checkProfileOnline)
        return;

    setupDirInfo(settings->profilePath());

    auto timer = new QTimer(this);
    timer->setInterval(2000);
    timer->start();

    connect(timer, &QTimer::timeout, this, &DbErrorManager::checkProfileDir);
}

void DbErrorManager::setupDirInfo(const QString &path)
{
    m_confDir.setPath(path);
    m_confDir.open();
}
