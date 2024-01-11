#include "dberrormanager.h"

#include <QLoggingCategory>
#include <QTimer>

#include <sqlite/sqlitedb.h>

#include <fortsettings.h>
#include <manager/windowmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

namespace {

const QLoggingCategory LC("dbErrorManager");

void sqliteLogHandler(void *context, int errCode, const char *message)
{
    qCWarning(LC) << "DB Error:" << errCode << qUtf8Printable(message);

    if (SqliteDb::isIoError(errCode)) {
        auto manager = static_cast<DbErrorManager *>(context);

        QMetaObject::invokeMethod(manager, &DbErrorManager::startPolling, Qt::QueuedConnection);
    }
}

}

DbErrorManager::DbErrorManager(QObject *parent) : QObject(parent) { }

void DbErrorManager::setUp()
{
    setupDriveMask();

    SqliteDb::setErrorLogCallback(sqliteLogHandler, /*context=*/this);
}

void DbErrorManager::checkDriveList()
{
    const quint32 driveMask = FileUtil::mountedDriveMask(m_driveMask);

    qCDebug(LC) << "Drive mounted state:" << Qt::hex << driveMask << "Expected:" << m_driveMask;

    if (m_driveMask == driveMask) {
        // Restart on profile drive mounted
        IoC<WindowManager>()->restart();
    }
}

void DbErrorManager::setupDriveMask()
{
    auto settings = IoC<FortSettings>();

    m_driveMask |= FileUtil::driveMaskByPath(settings->confFilePath());
}

void DbErrorManager::startPolling()
{
    if (m_polling)
        return;

    m_polling = true;

    setupPollingTimer();

    m_pollingTimer->start();

    qCDebug(LC) << "Start polling drive mounted state";
}

void DbErrorManager::setupPollingTimer()
{
    if (m_pollingTimer)
        return;

    m_pollingTimer = new QTimer(this);
    m_pollingTimer->setInterval(1000);

    connect(m_pollingTimer, &QTimer::timeout, this, &DbErrorManager::checkDriveList);
}
