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
}

bool AutoUpdateManager::isDownloading() const
{
    return downloader() && downloader()->started();
}

int AutoUpdateManager::bytesReceived() const
{
    return downloader() ? downloader()->buffer().size() : 0;
}

void AutoUpdateManager::setUp()
{
    setupTaskInfo();
}

void AutoUpdateManager::tearDown()
{
    finish();
}

void AutoUpdateManager::setupTaskInfo()
{
    auto taskManager = IoCDependency<TaskManager>();

    m_taskInfo = taskManager->taskInfoUpdateChecker();

    connect(taskManager, &TaskManager::appVersionDownloaded, this,
            &AutoUpdateManager::checkAutoUpdate);

    clearUpdateDir();

    checkAutoUpdate();
}

void AutoUpdateManager::setupDownloader()
{
    downloader()->setUrl(m_taskInfo->downloadUrl());

    connect(downloader(), &NetDownloader::startedChanged, this,
            &AutoUpdateManager::isDownloadingChanged);
    connect(downloader(), &NetDownloader::dataReceived, this,
            &AutoUpdateManager::bytesReceivedChanged);
}

void AutoUpdateManager::downloadFinished(bool success)
{
    if (success) {
        success = saveInstaller();
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

    const QString downloadUrl = m_taskInfo->downloadUrl();
    if (!downloadUrl.isEmpty() && m_taskInfo->isNewVersion()) {
        m_fileName = QUrl(downloadUrl).fileName();

        run();
    }
}

void AutoUpdateManager::clearUpdateDir()
{
    FileUtil::removePath(m_updatePath);
}

bool AutoUpdateManager::saveInstaller()
{
    const QByteArray fileData = downloader()->takeBuffer();
    if (fileData.size() != m_taskInfo->downloadSize())
        return false;

    return FileUtil::writeFileData(installerPath(), fileData);
}

bool AutoUpdateManager::runInstaller()
{
    QStringList args;

    auto settings = IoC<FortSettings>();

    if (!settings->isPortable()) {
        args << "/SILENT";
    }

    if (!settings->hasService()) {
        args << "/AUTORUN";
    } else {
        emit restartClients();
    }

    return QProcess::startDetached(installerPath(), args);
}
