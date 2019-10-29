#ifndef TASKUPDATECHECKER_H
#define TASKUPDATECHECKER_H

#include "taskdownloader.h"

class TaskUpdateChecker : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskUpdateChecker(QObject *parent = nullptr);

    QString version() const { return m_version; }

    QString releaseText() const;

protected:
    void setupDownloader() override;

protected slots:
    void downloadFinished(bool success) override;

private:
    bool parseBuffer(const QByteArray &buffer);

private:
    QString m_version;

    QString m_releaseName;
    QString m_publishedAt;
    QString m_releaseNotes;

    QString m_downloadUrl;
    int m_downloadSize;
    int m_downloadCount;
};

#endif // TASKUPDATECHECKER_H
