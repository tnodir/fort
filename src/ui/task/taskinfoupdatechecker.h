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
    QString downloadUrl() const { return m_downloadUrl; }
    QString releaseText() const { return m_releaseText; }

    QByteArray data() const override;
    void setData(const QByteArray &data) override;

    TaskUpdateChecker *updateChecker() const;

public slots:
    bool processResult(bool success) override;

private:
    void emitAppVersionUpdated();

private:
    QString m_version;
    QString m_downloadUrl;
    QString m_releaseText;
};

#endif // TASKINFOUPDATECHECKER_H
