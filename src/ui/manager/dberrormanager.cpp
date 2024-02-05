#include "dberrormanager.h"

#include <QLoggingCategory>
#include <QTimer>

#include <sqlite/sqlitedb.h>

#include <fortmanager.h>
#include <fortsettings.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

namespace {

const QLoggingCategory LC("manager.dbError");

void sqliteLogHandler(void *context, int errCode, const char *message)
{
    const auto messageLine =
            QString("%1: %2").arg(QString::number(errCode), qUtf8Printable(message));

    if (SqliteDb::isDebugError(errCode)) {
        qCDebug(LC) << messageLine;
    } else {
        qCWarning(LC) << messageLine;
    }

    if (SqliteDb::isIoError(errCode)) {
        auto manager = static_cast<DbErrorManager *>(context);

        QMetaObject::invokeMethod(manager, &DbErrorManager::onDbIoError, Qt::QueuedConnection);
    }
}

}

DbErrorManager::DbErrorManager(QObject *parent) : QObject(parent) { }

void DbErrorManager::setUp()
{
    auto settings = IoC<FortSettings>();

    // Drive Mask
    m_driveMask |= FileUtil::driveMaskByPath(settings->confFilePath());

    // SQLite Log Callback
    SqliteDb::setErrorLogCallback(sqliteLogHandler, /*context=*/this);
}

void DbErrorManager::onDbIoError()
{
    if (m_polling)
        return;

    m_polling = true;

    checkDriveList();
}

void DbErrorManager::checkDriveList()
{
    const quint32 driveMask = FileUtil::mountedDriveMask(m_driveMask);

    if (m_driveMask == driveMask) {
        // Restart on profile drive mounted
        IoC<FortManager>()->processRestartRequired(tr("Profile's drive mounted"));
        return;
    }

    qCDebug(LC) << "Drive mounted state:" << Qt::hex << driveMask << "Expected:" << m_driveMask;

    QTimer::singleShot(1000, this, &DbErrorManager::checkDriveList);
}
