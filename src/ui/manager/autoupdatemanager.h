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

    void setUp() override;
    void tearDown() override;

signals:
    void restartClients();

protected:
    void setupTaskInfo();

    void setupDownloader() override;

protected slots:
    void downloadFinished(bool success) override;

    void checkAutoUpdate();

    void clearUpdateDir();

    bool runInstaller();
    void prepareInstaller(QStringList &args) const;

private:
    static QString getDownloadUrl();

private:
    QString m_updatePath;
    QString m_fileName;

    TaskInfoUpdateChecker *m_taskInfo = nullptr;
};

#endif // AUTOUPDATEMANAGER_H
