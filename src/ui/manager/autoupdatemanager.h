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

    virtual bool isDownloading() const;
    virtual int bytesReceived() const;

    void setUp() override;
    void tearDown() override;

signals:
    void isDownloadingChanged();
    void bytesReceivedChanged();

    void restartClients();

public slots:
    bool runInstaller();

protected:
    void setupTaskInfo();

    void setupDownloader() override;

protected slots:
    void downloadFinished(bool success) override;

    void checkAutoUpdate();

private:
    void clearUpdateDir();

    bool saveInstaller();

    QString installerPath() const { return m_updatePath + m_fileName; }

    static QString getDownloadUrl();

private:
    QString m_updatePath;
    QString m_fileName;

    TaskInfoUpdateChecker *m_taskInfo = nullptr;
};

#endif // AUTOUPDATEMANAGER_H
