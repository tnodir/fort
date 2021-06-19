#include "taskdownloader.h"

#include "../util/net/netdownloader.h"

TaskDownloader::TaskDownloader(QObject *parent) : TaskWorker(parent) { }

void TaskDownloader::run()
{
    m_downloader = new NetDownloader(this);

    connect(m_downloader, &NetDownloader::finished, this, &TaskDownloader::downloadFinished);

    setupDownloader();

    if (m_downloader) {
        m_downloader->start();
    }
}

void TaskDownloader::finish(bool success)
{
    if (!m_downloader)
        return;

    m_downloader->disconnect(this); // to avoid recursive call on abort()

    m_downloader->finish();
    m_downloader->deleteLater();
    m_downloader = nullptr;

    emit finished(success);
}
