#include "autoupdatemanager.h"

#include <QFileInfo>
#include <QProcess>

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
    auto taskManager = IoCDependency<TaskManager>();
    auto taskInfo = taskManager->taskInfoUpdateChecker();

    connect(taskManager, &TaskManager::appVersionDownloaded, this,
            [=, this] { setupByTaskInfo(taskInfo); });

    setupByTaskInfo(taskInfo);

    if (!isDownloaded()) {
        clearUpdateDir();
    }
}

void AutoUpdateManager::tearDown()
{
    finish();
}

bool AutoUpdateManager::startDownload()
{
    if (downloader())
        return false;

    run();

    return true;
}

void AutoUpdateManager::setupDownloader()
{
    setIsDownloaded(false);

    downloader()->setUrl(m_downloadUrl);

    connect(downloader(), &NetDownloader::startedChanged, this,
            &AutoUpdateManager::isDownloadingChanged);
    connect(downloader(), &NetDownloader::dataReceived, this,
            &AutoUpdateManager::bytesReceivedChanged);
}

void AutoUpdateManager::downloadFinished(bool success)
{
    if (success) {
        success = saveInstaller();
        setIsDownloaded(success);
    }

    finish(success);
}

void AutoUpdateManager::setupByTaskInfo(TaskInfoUpdateChecker *taskInfo)
{
    m_downloadUrl = taskInfo->downloadUrl();
    if (m_downloadUrl.isEmpty())
        return;

    m_fileName = QUrl(m_downloadUrl).fileName();
    m_downloadSize = taskInfo->downloadSize();

    const QFileInfo fi(installerPath());
    setIsDownloaded(fi.size() == m_downloadSize);
}

void AutoUpdateManager::clearUpdateDir()
{
    auto settings = IoC<FortSettings>();

    if (settings->isMaster()) {
        FileUtil::removePath(m_updatePath);
    }
}

bool AutoUpdateManager::saveInstaller()
{
    const QByteArray fileData = downloader()->takeBuffer();
    if (fileData.size() != m_downloadSize)
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
