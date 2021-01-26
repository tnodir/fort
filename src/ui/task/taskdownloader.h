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

signals:

public slots:
    void run() override;
    void abort(bool success = false) override;

protected slots:
    virtual void downloadFinished(bool success) = 0;

private:
    NetDownloader *m_downloader = nullptr;
};

#endif // TASKDOWNLOADER_H
