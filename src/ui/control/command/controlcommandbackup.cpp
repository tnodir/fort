#include "controlcommandbackup.h"

#include <conf/confmanager.h>
#include <util/ioc/ioccontainer.h>

namespace {

enum BackupAction : quint32 {
    BackupActionNone = 0,
    BackupActionExport = (1 << 0),
    BackupActionImport = (1 << 1),
};

BackupAction backupActionByText(const QString &commandText)
{
    if (commandText == "export")
        return BackupActionExport;

    if (commandText == "import")
        return BackupActionImport;

    return BackupActionNone;
}

bool processCommandBackupAction(BackupAction backupAction, const QString &dirPath)
{
    switch (backupAction) {
    case BackupActionExport: {
        return IoC<ConfManager>()->exportBackup(dirPath);
    }
    case BackupActionImport: {
        return IoC<ConfManager>()->importBackup(dirPath);
    }
    default:
        return false;
    }
}

}

bool ControlCommandBackup::processCommand(const ProcessCommandArgs &p)
{
    const BackupAction backupAction = backupActionByText(p.args.value(0).toString());
    if (backupAction == BackupActionNone) {
        p.errorMessage = "Usage: backup export|import <dir-path>";
        return false;
    }

    if (!checkCommandActionPassword(p, backupAction))
        return false;

    const QString dirPath = p.args.value(1).toString();

    return processCommandBackupAction(backupAction, dirPath);
}
