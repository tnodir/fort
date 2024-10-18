#ifndef TASKDOWNLOADER_H
#define TASKDOWNLOADER_H

#include "taskworker.h"

class NetDownloader;

class TaskDownloader : public TaskWorker
{
    Q_OBJECT

public:
    explicit TaskDownloader(QObject *parent = nullptr);

    NetDownloader *downloader() const { return m_downloader; }

protected:
    void setDownloadMaxTryCount(quint16 v) { m_downloadMaxTryCount = v; }

    virtual void setupDownloader();

public slots:
    void run() override;
    void finish(bool success = false) override;

protected slots:
    virtual void downloadFinished(const QByteArray &data, bool success) = 0;

    void startDownloader();

private:
    void onFinished(const QByteArray &data, bool success);

    void createDownloader();
    void deleteDownloader();

private:
    quint16 m_downloadMaxTryCount = 0;
    quint16 m_downloadTryCount = 0;

    NetDownloader *m_downloader = nullptr;
};

#endif // TASKDOWNLOADER_H
