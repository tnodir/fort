#include "taskdownloader.h"

#include "../util/net/netdownloader.h"

TaskDownloader::TaskDownloader(QObject *parent) :
    TaskWorker(parent),
    m_downloader(nullptr)
{
}

void TaskDownloader::run()
{
    m_downloader = new NetDownloader(this);

    connect(m_downloader, &NetDownloader::finished,
            this, &TaskDownloader::downloadFinished);

    setupDownloader();

    downloader()->start();
}

void TaskDownloader::abort(bool success)
{
    if (!m_downloader) return;

    m_downloader->disconnect(this);  // to avoid recursive call on abort()

    m_downloader->abort();
    m_downloader->deleteLater();
    m_downloader = nullptr;

    emit finished(success);
}
