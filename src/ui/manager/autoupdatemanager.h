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
    explicit AutoUpdateManager(const QString &cachePath, QObject *parent = nullptr);

    bool isNewVersion() const { return m_isNewVersion; }

    bool isDownloaded() const { return m_isDownloaded; }
    void setIsDownloaded(bool v) { m_isDownloaded = v; }

    bool isDownloading() const { return m_isDownloading; }
    void setIsDownloading(bool v);

    int downloadSize() const { return m_downloadSize; }

    virtual int bytesReceived() const;

    void setUp() override;
    void tearDown() override;

public slots:
    virtual bool startDownload();

    bool runInstaller();

signals:
    void isDownloadingChanged(bool downloading);
    void bytesReceivedChanged(int size);

    void restartClients(const QString &installerPath);

protected:
    void setupDownloader() override;

protected slots:
    void downloadFinished(const QByteArray &data, bool success) override;

private:
    void setupByTaskInfo(TaskInfoUpdateChecker *taskInfo);

    void clearUpdateDir();

    bool saveInstaller(const QByteArray &fileData);

    QString installerPath() const { return m_updatePath + m_fileName; }

    static QStringList installerArgs(FortSettings *settings);

private:
    bool m_isNewVersion : 1 = false;
    bool m_isDownloaded : 1 = false;
    bool m_isDownloading : 1 = false;

    QString m_updatePath;

    QString m_fileName;
    QString m_downloadUrl;
    int m_downloadSize = 0;
};

#endif // AUTOUPDATEMANAGER_H
