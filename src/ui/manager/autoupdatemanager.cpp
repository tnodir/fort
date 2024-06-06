#include "autoupdatemanager.h"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QProcess>
#include <QTimer>

#include <fortsettings.h>
#include <manager/servicemanager.h>
#include <rpc/rpcmanager.h>
#include <task/taskinfoupdatechecker.h>
#include <task/taskmanager.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netdownloader.h>
#include <util/osutil.h>

namespace {

const QLoggingCategory LC("manager.autoUpdate");

}

AutoUpdateManager::AutoUpdateManager(const QString &cachePath, QObject *parent) :
    TaskDownloader(parent), m_updatePath(cachePath + "update/")
{
}

void AutoUpdateManager::setFlags(Flags v)
{
    if (m_flags != v) {
        m_flags = v;
        emit flagsChanged();
    }
}

void AutoUpdateManager::setFlag(Flag v, bool on)
{
    Flags flags = m_flags;
    flags.setFlag(v, on);
    setFlags(flags);
}

void AutoUpdateManager::setFileName(const QString &v)
{
    if (m_fileName != v) {
        m_fileName = v;
        emit fileNameChanged();
    }
}

int AutoUpdateManager::bytesReceived() const
{
    return downloader() ? downloader()->buffer().size() : m_downloadSize;
}

void AutoUpdateManager::setUp()
{
    setupManager();
    setupRestart();
}

void AutoUpdateManager::tearDown()
{
    finish();
}

void AutoUpdateManager::setupManager()
{
    auto taskManager = IoCDependency<TaskManager>();
    auto taskInfo = taskManager->taskInfoUpdateChecker();

    connect(taskManager, &TaskManager::appVersionUpdated, this,
            [=, this] { setupByTaskInfo(taskInfo); });

    setupByTaskInfo(taskInfo);

    // Wait Installer's exit
    QTimer::singleShot(500, this, &AutoUpdateManager::clearUpdateDir);
}

void AutoUpdateManager::setupRestart()
{
    if (IoC<FortSettings>()->isService()) {
        connect(IoC<ServiceManager>(), &ServiceManager::stopRestartingRequested, this,
                &AutoUpdateManager::onRestartClientsRequested);
    } else {
        if (!OsUtil::registerAppRestart()) {
            qCWarning(LC) << "Restart registration error";
        }
    }
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
    setIsNewVersion(taskInfo->isNewVersion());
    if (!isNewVersion())
        return;

    m_downloadUrl = taskInfo->downloadUrl();
    m_downloadSize = taskInfo->downloadSize();
    if (m_downloadUrl.isEmpty() || m_downloadSize <= 0)
        return;

    setFileName(QUrl(m_downloadUrl).fileName());

    const QFileInfo fi(installerPath());
    const bool downloaded = (fi.exists() && fi.size() == m_downloadSize);

    qCDebug(LC) << "Check:" << taskInfo->version() << "downloaded:" << downloaded;

    setIsDownloaded(downloaded);
}

void AutoUpdateManager::onRestartClientsRequested(bool restarting)
{
    if (restarting) {
        OsUtil::beginRestartClients();
    }

    emit restartClients(restarting);
}

void AutoUpdateManager::clearUpdateDir()
{
    if (isDownloaded())
        return;

    if (!IoC<FortSettings>()->isMaster())
        return;

    if (!FileUtil::pathExists(m_updatePath))
        return;

    if (FileUtil::removePath(m_updatePath)) {
        qCDebug(LC) << "Dir removed:" << m_updatePath;
    } else {
        qCWarning(LC) << "Dir remove error:" << m_updatePath;
    }
}

bool AutoUpdateManager::saveInstaller(const QByteArray &fileData)
{
    if (fileData.size() != m_downloadSize) {
        qCWarning(LC) << "Installer size mismatch error:" << fileData.size()
                      << "Expected:" << m_downloadSize << fileName();
        return false;
    }

    if (!FileUtil::writeFileData(installerPath(), fileData)) {
        qCWarning(LC) << "Installer save error:" << installerPath();
        return false;
    }

    qCDebug(LC) << "Installer saved:" << installerPath();

    return true;
}

bool AutoUpdateManager::runInstaller()
{
    auto settings = IoC<FortSettings>();

    const QString installerPath = this->installerPath();
    const QStringList args = installerArgs(settings);

    if (!QProcess::startDetached(installerPath, args)) {
        qCWarning(LC) << "Run Installer error:" << installerPath << args;
        return false;
    }

    qCDebug(LC) << "Run Installer:" << installerPath << args;

    if (settings->hasService()) {
        emit restartClients(/*restarting=*/true);
    }

    if (settings->isMaster()) {
        OsUtil::quit("new version install");
    }

    return true;
}

QStringList AutoUpdateManager::installerArgs(FortSettings *settings)
{
    QStringList args = { "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NOCANCEL" };

    if (settings->forceDebug()) {
        args << "/LOG";
    }

    if (settings->isPortable()) {
        const QString appPath = FileUtil::toNativeSeparators(FileUtil::appBinLocation());

        args << QString("/PATH=%1").arg(appPath);

        // Portable Tasks
        args << installerPortableTasks(settings);
    }

    if (!settings->hasService()) {
        args << "/LAUNCH";
    }

    return args;
}

QString AutoUpdateManager::installerPortableTasks(FortSettings *settings)
{
    QString tasks = "/TASKS=portable";

    if (settings->hasService()) {
        tasks += ",service";
    }

    return tasks;
}

void AutoUpdateManager::onRestartClient(bool restarting)
{
    if (restarting) {
        OsUtil::restartClient();
    } else {
        OsUtil::quit("uninstall");
    }
}
