#include "taskuzonline.h"

#include "../util/httpdownloader.h"

TaskUzonline::TaskUzonline(QObject *parent) :
    TaskTasix(parent)
{
}

#include <QDebug>
QString TaskUzonline::parseUzonlineBufer(const QByteArray &buffer)
{
    qDebug() << buffer.length() << buffer;
    return QString();
}

void TaskUzonline::startDownloader() const
{
    downloader()->get(QUrl("https://alltor.me/viewtopic.php?p=1345405"));
}
