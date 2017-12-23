#ifndef TASKUPDATECHECKER_H
#define TASKUPDATECHECKER_H

#include "taskdownloader.h"

class TaskUpdateChecker : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskUpdateChecker(QObject *parent = nullptr);

protected:
    void setupDownloader() override;

signals:

public slots:
    bool processResult(FortManager *fortManager) override;

protected slots:
    void downloadFinished(bool success) override;

private:
    bool parseBuffer(const QByteArray &buffer);

    QString successMessage() const;

private:
    QString m_releaseName;
    QString m_publishedAt;
    QString m_releaseNotes;

    QString m_downloadUrl;
    int m_downloadSize;
    int m_downloadCount;
};

#endif // TASKUPDATECHECKER_H
