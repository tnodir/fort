#include "taskupdatechecker.h"

#include <QJsonDocument>
#include <QVariant>

#include "../../common/version.h"
#ifndef TASK_TEST
#include "../fortmanager.h"
#endif
#include "../util/net/netdownloader.h"

TaskUpdateChecker::TaskUpdateChecker(QObject *parent) :
    TaskDownloader(parent)
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

bool TaskUpdateChecker::processResult(FortManager *fortManager)
{
#ifndef TASK_TEST
    fortManager->showTrayMessage(m_releaseName);
#else
    Q_UNUSED(fortManager)
#endif

    return true;
}

bool TaskUpdateChecker::parseBuffer(const QByteArray &buffer)
{
    QJsonParseError jsonParseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(
                buffer, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        //TODO: jsonParseError.errorString()
        return false;
    }

    const QVariantMap map = jsonDoc.toVariant().toMap();

    // Check draft/prerelease
    if (map["draft"].toBool() || map["prerelease"].toBool())
        return false;

    // Check version (eg. "v1.4.0")
    const QString tagName = map["tag_name"].toString();
    if (tagName.midRef(1) == APP_VERSION_STR)
        return false;

    m_releaseName = map["name"].toString();  // eg. "Fort Firewall v1.4.0"
    m_publishedAt = map["published_at"].toString();  // eg. "2017-12-17T02:27:19Z"
    m_releaseNotes = map["body"].toString();  // ChangeLog

    // Assets
    const QVariantList assets = map["assets"].toList();
    if (assets.isEmpty())
        return false;

    // Assets
    const QVariantMap assetMap = assets.first().toMap();

    // eg. "https://github.com/tnodir/fort/releases/download/v1.4.0/FortFirewall-1.4.0.exe"
    m_downloadUrl = assetMap["browser_download_url"].toString();
    m_downloadSize = assetMap["size"].toInt();
    m_downloadCount = assetMap["download_count"].toInt();

    if (m_downloadUrl.isEmpty() || !m_downloadSize)
        return false;

    return true;
}
