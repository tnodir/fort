#include "taskupdatechecker.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QVariant>

#include "../../common/version.h"
#include "../util/net/netdownloader.h"
#include "../util/net/netutil.h"

TaskUpdateChecker::TaskUpdateChecker(QObject *parent) :
    TaskDownloader(parent),
    m_downloadSize(0),
    m_downloadCount(0)
{
}

void TaskUpdateChecker::setupDownloader()
{
    downloader()->setUrl(APP_UPDATES_API_URL);
}

void TaskUpdateChecker::downloadFinished(bool success)
{
    if (success) {
        success = parseBuffer(downloader()->buffer());
    }

    abort(success);
}

bool TaskUpdateChecker::parseBuffer(const QByteArray &buffer)
{
    QJsonParseError jsonParseError{};
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(
                buffer, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        // TODO: jsonParseError.errorString()
        return false;
    }

    const QVariantMap map = jsonDoc.toVariant().toMap();

    // Check version (eg. "v1.4.0")
    const QString tagName = map["tag_name"].toString();
    m_version = tagName.mid(1);
    if (m_version == APP_VERSION_STR)
        return false;

    // Check draft/prerelease
    if (map["draft"].toBool() || map["prerelease"].toBool())
        return false;

    // Check Assets
    const QVariantList assets = map["assets"].toList();
    if (assets.isEmpty())
        return false;

    m_releaseName = map["name"].toString();  // eg. "Fort Firewall v1.4.0"
    m_publishedAt = map["published_at"].toString();  // eg. "2017-12-17T02:27:19Z"

    m_releaseNotes = map["body"].toString();  // ChangeLog

    // Assets
    const QVariantMap assetMap = assets.first().toMap();

    // eg. "https://github.com/tnodir/fort/releases/download/v1.4.0/FortFirewall-1.4.0.exe"
    m_downloadUrl = assetMap["browser_download_url"].toString();
    m_downloadSize = assetMap["size"].toInt();
    m_downloadCount = assetMap["download_count"].toInt();

    return !m_downloadUrl.isEmpty() && m_downloadSize != 0;
}

QString TaskUpdateChecker::releaseText() const
{
    const QDateTime publishedTime = QDateTime::fromString(
                m_publishedAt, Qt::ISODate);

    QString releaseNotes = m_releaseNotes;
    releaseNotes.replace('\n', "<br/>");

    return "<a href=\"" + m_downloadUrl + "\">"
            + m_releaseName + "</a> (<i>"
            + publishedTime.toString("dd-MMM-yyyy hh:mm") + "</i>, "
            + NetUtil::formatDataSize(m_downloadSize)
            + ", #" + QString::number(m_downloadCount)
            + ")<br/>\nRelease Notes:<br/>\n"
            + releaseNotes;
}
