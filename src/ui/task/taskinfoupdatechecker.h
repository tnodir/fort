#ifndef TASKINFOUPDATECHECKER_H
#define TASKINFOUPDATECHECKER_H

#include "taskinfo.h"

class TaskUpdateChecker;

class TaskInfoUpdateChecker : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoUpdateChecker(TaskManager &taskManager);

    bool isNewVersion() const;
    QString version() const { return m_version; }
    QString releaseText() const { return m_releaseText; }
    QString downloadUrl() const { return m_downloadUrl; }
    int downloadSize() const { return m_downloadSize; }

    QByteArray data() const override;
    void setData(const QByteArray &data) override;

    TaskUpdateChecker *updateChecker() const;

public slots:
    bool processResult(bool success) override;

private:
    void emitAppVersionUpdated();

private:
    QString m_version;
    QString m_releaseText;

    QString m_downloadUrl;
    int m_downloadSize = 0;
};

#endif // TASKINFOUPDATECHECKER_H
