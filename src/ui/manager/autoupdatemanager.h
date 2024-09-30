#ifndef AUTOUPDATEMANAGER_H
#define AUTOUPDATEMANAGER_H

#include <QObject>

#include <task/taskdownloader.h>
#include <util/ioc/iocservice.h>

class FortSettings;
class IniOptions;
class TaskInfoUpdateChecker;

class AutoUpdateManager : public TaskDownloader, public IocService
{
    Q_OBJECT

public:
    enum Flag {
        NoFlag = 0,
        IsNewVersion = (1 << 0),
        IsDownloaded = (1 << 1),
        IsDownloading = (1 << 2),
    };
    Q_ENUM(Flag)
    Q_DECLARE_FLAGS(Flags, Flag)

    explicit AutoUpdateManager(const QString &updatePath, QObject *parent = nullptr);

    Flags flags() const { return m_flags; }
    void setFlags(Flags v);

    void setFlag(Flag v, bool on = true);
    constexpr bool testFlag(Flag v) const { return flags().testFlag(v); }

    bool isNewVersion() const { return testFlag(IsNewVersion); }
    void setIsNewVersion(bool on) { setFlag(IsNewVersion, on); }

    bool isDownloaded() const { return testFlag(IsDownloaded); }
    void setIsDownloaded(bool on) { setFlag(IsDownloaded, on); }

    bool isDownloading() const { return testFlag(IsDownloading); }
    void setIsDownloading(bool on) { setFlag(IsDownloading, on); }

    bool hasUpdate() const { return isNewVersion() || keepCurrentVersion(); }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &v);

    virtual int bytesReceived() const;

    void setUp() override;
    void tearDown() override;

    const QString &updatePath() const { return m_updatePath; }
    QString installerPath() const { return updatePath() + m_fileName; }

public slots:
    virtual bool startDownload();

    virtual bool runInstaller();

    void onRestartClient(bool restarting);

signals:
    void keepCurrentVersionChanged();
    void flagsChanged();
    void bytesReceivedChanged(int size);
    void fileNameChanged();

    void restartClients(bool restarting);

protected:
    bool keepCurrentVersion() const { return m_keepCurrentVersion; }
    void setKeepCurrentVersion(bool v);

    bool autoDownload() const { return m_autoDownload; }
    void setAutoDownload(bool v) { m_autoDownload = v; }

    virtual void setupManager();
    void setupConfManager();
    void setupTaskManager();

    void setupRestart();

    void setupDownloader() override;

protected slots:
    void downloadFinished(const QByteArray &data, bool success) override;

private:
    void setupByTaskInfo(TaskInfoUpdateChecker *taskInfo);

    void onRestartClientsRequested(bool restarting);

    void clearUpdateDir();

    bool saveInstaller(const QByteArray &fileData);

    void setupByConf(const IniOptions &ini);

    static QStringList installerArgs(FortSettings *settings);
    static QString installerPortableTasks(FortSettings *settings);

private:
    bool m_keepCurrentVersion : 1 = false;
    bool m_autoDownload : 1 = false;

    Flags m_flags = NoFlag;

    QString m_updatePath;

    QString m_fileName;
    QString m_downloadUrl;
    int m_downloadSize = 0;
    int m_downloadTryCount = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AutoUpdateManager::Flags)

#endif // AUTOUPDATEMANAGER_H
