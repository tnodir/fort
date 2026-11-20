#include "backupcontroller.h"

#include <QMessageBox>

#include <conf/confappmanager.h>
#include <conf/confmanager.h>
#include <form/dialog/dialogutil.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <stat/statmanager.h>
#include <util/fileutil.h>

using namespace Fort;

namespace {

QString selectBackupDir(const QString &title)
{
    return DialogUtil::getExistingDir(title, FileUtil::documentsLocation());
}

}

BackupController::BackupController(QObject *parent) : BaseController(parent) { }

void BackupController::exportBackup()
{
    const auto path = selectBackupDir(tr("Export Backup"));
    if (path.isEmpty())
        return;

    statManager()->exportBackup(path);

    const bool ok = confManager()->exportBackup(path);

    const auto icon = ok ? QMessageBox::Information : QMessageBox::Critical;
    const auto title = ok ? tr("Backup Exported Successfully") : tr("Cannot Export Backup");

    windowManager()->showMessageBox(icon, title, tr("Export Backup"));
}

void BackupController::importBackup(bool onlyNewApps)
{
    const auto path = selectBackupDir(tr("Import Backup"));
    if (path.isEmpty())
        return;

    bool ok;
    if (onlyNewApps) {
        ok = confAppManager()->importAppsBackup(path);
    } else {
        statManager()->importBackup(path);

        ok = confManager()->importBackup(path);
    }

    const auto icon = ok ? QMessageBox::Information : QMessageBox::Critical;
    const auto title = ok ? tr("Backup Imported Successfully") : tr("Cannot Import Backup");

    windowManager()->showMessageBox(icon, title, tr("Import Backup"));
}

void BackupController::confirmImportBackup()
{
    windowManager()->showConfirmBox(
            this, [&] { importBackup(); },
            tr("All Options and Programs will be replaced after successful import. Continue?\n\n"
               "Make sure that you have a fresh backup."),
            tr("Import Backup"));
}

void BackupController::confirmImportAppsBackup()
{
    windowManager()->showConfirmBox(
            this, [&] { importBackup(/*onlyNewApps=*/true); },
            tr("Only new Programs will be added after successful import. Continue?"),
            tr("Import Backup"));
}
