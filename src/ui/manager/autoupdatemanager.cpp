#include "autoupdatemanager.h"

#include <QProcess>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <conf/inioptions.h>
#include <fortsettings.h>
#include <rpc/rpcmanager.h>
#include <task/taskinfoupdatechecker.h>
#include <task/taskmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netdownloader.h>

AutoUpdateManager::AutoUpdateManager(const QString &cachePath, QObject *parent) :
    TaskDownloader(parent), m_updatePath(cachePath + "update/")
{
    clearUpdateDir();
}

void AutoUpdateManager::setUp()
{
    auto taskManager = IoCDependency<TaskManager>();

    m_taskInfo = taskManager->taskInfoUpdateChecker();

    connect(taskManager, &TaskManager::appVersionDownloaded, this,
            &AutoUpdateManager::checkAutoUpdate);

    checkAutoUpdate();
}

void AutoUpdateManager::tearDown()
{
    finish();
}

void AutoUpdateManager::setupDownloader()
{
    downloader()->setUrl(m_taskInfo->downloadUrl());
}

void AutoUpdateManager::downloadFinished(bool success)
{
    if (success) {
        success = runInstaller();
    }

    finish(success);
}

void AutoUpdateManager::checkAutoUpdate()
{
    if (downloader())
        return;

    auto confManager = IoCDependency<ConfManager>();
    if (!confManager->conf()->ini().updaterAutoUpdate())
        return;

    if (m_taskInfo->isNewVersion() && !m_taskInfo->downloadUrl().isEmpty()) {
        run();
    }
}

void AutoUpdateManager::clearUpdateDir()
{
    FileUtil::removePath(m_updatePath);
}

bool AutoUpdateManager::runInstaller()
{
    const QString fileName = QUrl(downloader()->url()).fileName();
    const QString exePath = m_updatePath + fileName;

    if (!FileUtil::writeFileData(exePath, downloader()->takeBuffer()))
        return false;

    QStringList args;
    fillInstallerArgs(args);

    if (!QProcess::startDetached(exePath, args))
        return false;

    return true;
}

void AutoUpdateManager::fillInstallerArgs(QStringList &args) const
{
    auto settings = IoC<FortSettings>();

    if (!settings->isPortable()) {
        args << "/SILENT";
    }

    if (!settings->hasService()) {
        args << "/AUTORUN";
    } else if (settings->isService()) {
        IoC<RpcManager>()->invokeOnClients(Control::Rpc_RpcManager_restartClient);
    }
}
