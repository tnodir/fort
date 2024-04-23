#ifndef AUTOUPDATEMANAGER_H
#define AUTOUPDATEMANAGER_H

#include <QObject>

#include <task/taskdownloader.h>
#include <util/ioc/iocservice.h>

class TaskInfoUpdateChecker;

class AutoUpdateManager : public TaskDownloader, public IocService
{
    Q_OBJECT

public:
    explicit AutoUpdateManager(const QString &cachePath, QObject *parent = nullptr);

    bool isDownloaded() const { return m_isDownloaded; }

    virtual bool isDownloading() const;
    virtual int bytesReceived() const;

    void setUp() override;
    void tearDown() override;

public slots:
    virtual bool startDownload();

    bool runInstaller();

signals:
    void isDownloadingChanged();
    void bytesReceivedChanged();

    void restartClients();

protected:
    void setIsDownloaded(bool v) { m_isDownloaded = v; }

    void setupDownloader() override;

protected slots:
    void downloadFinished(bool success) override;

private:
    void setupByTaskInfo(TaskInfoUpdateChecker *taskInfo);

    void clearUpdateDir();

    bool saveInstaller();

    QString installerPath() const { return m_updatePath + m_fileName; }

    static QString getDownloadUrl();

private:
    bool m_isDownloaded = false;

    QString m_updatePath;

    QString m_fileName;
    QString m_downloadUrl;
    int m_downloadSize = 0;
};

#endif // AUTOUPDATEMANAGER_H
