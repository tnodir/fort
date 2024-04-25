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
    virtual void setupDownloader() = 0;

public slots:
    void run() override;
    void finish(bool success = false) override;

protected slots:
    virtual void downloadFinished(const QByteArray &data, bool success) = 0;

private:
    void createDownloader();
    void deleteDownloader();

    void startDownloader();

private:
    NetDownloader *m_downloader = nullptr;
};

#endif // TASKDOWNLOADER_H
