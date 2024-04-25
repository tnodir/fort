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
#include <util/osutil.h>

AutoUpdateManager::AutoUpdateManager(const QString &cachePath, QObject *parent) :
    TaskDownloader(parent), m_updatePath(cachePath + "update/")
{
}

void AutoUpdateManager::setIsDownloading(bool v)
{
    if (m_isDownloading != v) {
        m_isDownloading = v;
        emit isDownloadingChanged(v);
    }
}

int AutoUpdateManager::bytesReceived() const
{
    return downloader() ? downloader()->buffer().size() : 0;
}

void AutoUpdateManager::setUp()
{
    auto taskManager = IoCDependency<TaskManager>();
    auto taskInfo = taskManager->taskInfoUpdateChecker();

    connect(taskManager, &TaskManager::appVersionUpdated, this,
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
    downloader()->setUrl(m_downloadUrl);

    connect(downloader(), &NetDownloader::dataReceived, this,
            &AutoUpdateManager::bytesReceivedChanged);

    setIsDownloaded(false);
    setIsDownloading(true);
}

void AutoUpdateManager::downloadFinished(const QByteArray &data, bool success)
{
    if (success) {
        success = saveInstaller(data);

        setIsDownloaded(success);
    }

    setIsDownloading(false);

    finish(success);
}

void AutoUpdateManager::setupByTaskInfo(TaskInfoUpdateChecker *taskInfo)
{
    m_isNewVersion = taskInfo->isNewVersion();
    if (!m_isNewVersion)
        return;

    m_downloadUrl = taskInfo->downloadUrl();
    m_downloadSize = taskInfo->downloadSize();
    if (m_downloadUrl.isEmpty() || m_downloadSize <= 0)
        return;

    m_fileName = QUrl(m_downloadUrl).fileName();

    const QFileInfo fi(installerPath());
    const bool downloaded = (fi.exists() && fi.size() == m_downloadSize);

    setIsDownloaded(downloaded);
}

void AutoUpdateManager::clearUpdateDir()
{
    auto settings = IoC<FortSettings>();

    if (settings->isMaster()) {
        FileUtil::removePath(m_updatePath);
    }
}

bool AutoUpdateManager::saveInstaller(const QByteArray &fileData)
{
    if (fileData.size() != m_downloadSize)
        return false;

    return FileUtil::writeFileData(installerPath(), fileData);
}

bool AutoUpdateManager::runInstaller()
{
    auto settings = IoC<FortSettings>();

    const QString installerPath = this->installerPath();

    if (!QProcess::startDetached(installerPath, installerArgs(settings)))
        return false;

    if (settings->hasService()) {
        emit restartClients(installerPath);
    } else {
        OsUtil::quit("new version install");
    }

    return true;
}

QStringList AutoUpdateManager::installerArgs(FortSettings *settings)
{
    QStringList args;

    if (!settings->isPortable()) {
        args << "/SILENT";
    }

    if (!settings->hasService()) {
        args << "/LAUNCH";
    }

    return args;
}
