#ifndef TASKINFOUPDATECHECKER_H
#define TASKINFOUPDATECHECKER_H

#include "taskinfo.h"

class TaskInfoUpdateChecker : public TaskInfo
{
    Q_OBJECT

public:
    explicit TaskInfoUpdateChecker(QObject *parent = nullptr);

    QString version() const { return m_version; }

    QString infoText() const override;

    QByteArray data() const override;
    void setData(const QByteArray &data) override;

public slots:
    bool processResult(FortManager *fortManager, bool success) override;

private:
    QString m_version;
    QString m_releaseText;
};

#endif // TASKINFOUPDATECHECKER_H
