#include "drivelistmanager.h"

#include <QLoggingCategory>
#include <QThreadPool>

#include <fortsettings.h>
#include <manager/nativeeventfilter.h>
#include <manager/servicemanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

namespace {
const QLoggingCategory LC("manager.driveList");
}

DriveListManager::DriveListManager(QObject *parent) : QObject(parent)
{
    connect(&m_driveListTimer, &QTimer::timeout, this, &DriveListManager::populateDriveMask);
}

void DriveListManager::setUp()
{
    auto settings = IoC<FortSettings>();

    m_isUserAdmin = settings->isUserAdmin();

    if (settings->isService()) {
        connect(IoC<ServiceManager>(), &ServiceManager::driveListChanged, this,
                &DriveListManager::onDriveListChanged);
    } else {
        connect(IoC<NativeEventFilter>(), &NativeEventFilter::driveListChanged, this,
                &DriveListManager::onDriveListChanged);
    }

    populateDriveMask();
}

void DriveListManager::onDriveListChanged()
{
    m_driveListTimer.startTrigger();
}

void DriveListManager::populateDriveMask()
{
    if (!m_isUserAdmin) {
        setDriveMask(FileUtil::driveMask());
        return;
    }

    QThreadPool::globalInstance()->start([this] {
        const quint32 driveMask = FileUtil::mountedDriveMask(quint32(-1));

        QMetaObject::invokeMethod(
                this, [this, driveMask] { setDriveMask(driveMask); }, Qt::QueuedConnection);
    });
}

void DriveListManager::setDriveMask(quint32 driveMask)
{
    if (m_driveMask == driveMask)
        return;

    const quint32 oldDriveMask = m_driveMask;
    m_driveMask = driveMask;

    const quint32 addedMask = (driveMask & ~oldDriveMask);
    const quint32 removedMask = (~driveMask & oldDriveMask);

    qCDebug(LC) << Qt::hex << "driveMask:" << driveMask << "addedMask:" << addedMask
                << "removedMask:" << removedMask;

    if (oldDriveMask != 0) {
        emit driveMaskChanged(addedMask, removedMask);
    }
}
