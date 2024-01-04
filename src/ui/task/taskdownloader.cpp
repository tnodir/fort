#include "taskdownloader.h"

#include <util/net/netdownloader.h>

TaskDownloader::TaskDownloader(QObject *parent) : TaskWorker(parent) { }

void TaskDownloader::run()
{
    createDownloader();

    setupDownloader();

    startDownloader();
}

void TaskDownloader::finish(bool success)
{
    if (!m_downloader)
        return;

    deleteDownloader();

    emit finished(success);
}

void TaskDownloader::createDownloader()
{
    if (m_downloader)
        return;

    m_downloader = new NetDownloader(this);

    connect(m_downloader, &NetDownloader::finished, this, &TaskDownloader::downloadFinished);
}

void TaskDownloader::deleteDownloader()
{
    if (!m_downloader)
        return;

    m_downloader->disconnect(this); // to avoid recursive call on abort()

    m_downloader->finish();

    m_downloader->deleteLater();
    m_downloader = nullptr;
}

void TaskDownloader::startDownloader()
{
    if (m_downloader) {
        m_downloader->start();
    }
}
