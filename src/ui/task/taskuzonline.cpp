#include "taskuzonline.h"

#include <QRegularExpression>
#include <QVector>

#include "../util/net/netdownloader.h"

TaskUzonline::TaskUzonline(QObject *parent) :
    TaskTasix(parent)
{
}

void TaskUzonline::setupDownloader()
{
    downloader()->setUrl("https://alltor.me/viewtopic.php?p=1345405");
}

QStringList TaskUzonline::parseUzonlineBuffer(const QByteArray &buffer)
{
    const int startPos = buffer.indexOf("=UZONLINE=START=");
    const int endPos = buffer.indexOf("=UZONLINE=END=", startPos + 1);
    if (startPos < 0 || endPos < 0)
        return QStringList();

    const QRegularExpression re(R"(([\d.]+)[^\d.]*-[^\d.]*([\d.]+))");

    QStringList list;

    // Parse lines
    const QString text = QString::fromLatin1(buffer.constData() + startPos,
                                             endPos - startPos);

    foreach (const QStringRef &line, text.splitRef(
                 "<br", QString::SkipEmptyParts)) {
        const QRegularExpressionMatch match = re.match(line);
        if (!match.hasMatch())
            continue;

        const QString ip1 = match.captured(1);
        const QString ip2 = match.captured(2);

        list.append(ip1 + '-' + ip2);
    }

    return list;
}
