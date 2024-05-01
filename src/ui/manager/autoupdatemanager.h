#ifndef AUTOUPDATEMANAGER_H
#define AUTOUPDATEMANAGER_H

#include <QObject>

#include <task/taskdownloader.h>
#include <util/ioc/iocservice.h>

class FortSettings;
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

    explicit AutoUpdateManager(const QString &cachePath, QObject *parent = nullptr);

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

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &v);

    virtual int bytesReceived() const;

    void setUp() override;
    void tearDown() override;

    QString installerPath() const { return m_updatePath + m_fileName; }

public slots:
    virtual bool startDownload();

    virtual bool runInstaller();

signals:
    void flagsChanged();
    void bytesReceivedChanged(int size);
    void fileNameChanged();

    void restartClients();

protected:
    virtual void setupManager();
    virtual void setupRestart();

    void setupDownloader() override;

protected slots:
    void downloadFinished(const QByteArray &data, bool success) override;

private:
    void setupByTaskInfo(TaskInfoUpdateChecker *taskInfo);

    void clearUpdateDir();

    bool saveInstaller(const QByteArray &fileData);

    static QStringList installerArgs(FortSettings *settings);

private:
    Flags m_flags = NoFlag;

    QString m_updatePath;

    QString m_fileName;
    QString m_downloadUrl;
    int m_downloadSize = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AutoUpdateManager::Flags)

#endif // AUTOUPDATEMANAGER_H
