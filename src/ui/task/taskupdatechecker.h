#ifndef TASKUPDATECHECKER_H
#define TASKUPDATECHECKER_H

#include "taskdownloader.h"

class TaskUpdateChecker : public TaskDownloader
{
    Q_OBJECT

public:
    explicit TaskUpdateChecker(QObject *parent = nullptr);

    QString version() const { return m_version; }
    QString downloadUrl() const { return m_downloadUrl; }
    int downloadSize() const { return m_downloadSize; }

    QString releaseText() const;

protected:
    void setupDownloader() override;

protected slots:
    void downloadFinished(const QByteArray &data, bool success) override;

private:
    bool parseBuffer(const QByteArray &buffer);

private:
    QString m_version;

    QString m_releaseName;
    QString m_publishedAt;
    QString m_releaseNotes;

    QString m_downloadUrl;
    int m_downloadSize = 0;
};

#endif // TASKUPDATECHECKER_H
