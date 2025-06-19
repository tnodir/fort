#include "drivelistworker.h"

#include <QLoggingCategory>

#include <common/fortioctl.h>
#include <util/osutil.h>

#define MOUNTMGR_DOS_DEVICE_NAME "\\\\.\\MountPointManager"

#define MOUNTMGRCONTROLTYPE 0x0000006D // 'm'

#define IOCTL_MOUNTMGR_CHANGE_NOTIFY                                                               \
    CTL_CODE(MOUNTMGRCONTROLTYPE, 8, METHOD_BUFFERED, FILE_READ_ACCESS)

namespace {
const QLoggingCategory LC("driveList.worker");
}

DriveListWorker::DriveListWorker(QObject *parent) : QObject(parent) { }

void DriveListWorker::run()
{
    OsUtil::setCurrentThreadName("DriveListWorker");

    if (!openDevice()) {
        qCWarning(LC) << "Can't open MountMgr device";
        return;
    }

    do {
        if (!readEpicNumber())
            break;

        emit driveListChanged();
    } while (!m_aborted);

    if (!m_aborted) {
        qCWarning(LC) << "MountMgr device error:" << OsUtil::errorMessage();
    }
}

void DriveListWorker::close()
{
    if (m_aborted)
        return;

    m_aborted = true;

    closeDevice();
}

bool DriveListWorker::openDevice()
{
    return device().open(MOUNTMGR_DOS_DEVICE_NAME);
}

void DriveListWorker::closeDevice()
{
    device().cancelIo();
    device().close();
}

bool DriveListWorker::readEpicNumber()
{
    char *buf = (char *) &m_epicNumber;
    qsizetype nr = 0;

    const bool success = device().ioctl(
            IOCTL_MOUNTMGR_CHANGE_NOTIFY, buf, sizeof(ULONG), buf, sizeof(ULONG), &nr);

    return success;
}
