#include "taskupdatechecker.h"

#include <QDateTime>
#include <QLoggingCategory>
#include <QVariant>

#include <fort_version.h>

#include <util/json/jsonutil.h>
#include <util/net/netdownloader.h>
#include <util/net/netutil.h>

namespace {

const QLoggingCategory LC("task.updateChecker");

const QString downloadUrlSuffix =
#if defined(Q_PROCESSOR_ARM)
        "-arm64.exe"
#elif defined(Q_PROCESSOR_X86_64)
        "-x86_64.exe"
#else
        "-x86.exe"
#endif
        ;

QVariantMap findDownloadableAsset(const QVariantList &assets)
{
    for (const QVariant &asset : assets) {
        const QVariantMap assetMap = asset.toMap();

        const QString downloadUrl = assetMap["browser_download_url"].toString();
        if (downloadUrl.endsWith(downloadUrlSuffix))
            return assetMap;
    }

    return assets.first().toMap();
}

}

TaskUpdateChecker::TaskUpdateChecker(QObject *parent) : TaskDownloader(parent) { }

QString TaskUpdateChecker::releaseText() const
{
    const QDateTime publishedTime = QDateTime::fromString(m_publishedAt, Qt::ISODate);

    return "[" + m_releaseName + "](" + APP_UPDATES_URL + "/tag/v" + m_version + ") (*"
            + publishedTime.toString("dd-MMM-yyyy hh:mm") + "*, "
            + NetUtil::formatDataSize(m_downloadSize) + ")\n\n*Release Notes:*\n" + m_releaseNotes;
}

void TaskUpdateChecker::setupDownloader()
{
    downloader()->setUrl(APP_UPDATES_API_URL);
}

void TaskUpdateChecker::downloadFinished(const QByteArray &data, bool success)
{
    if (success) {
        success = parseBuffer(data);
    }

    finish(success);
}

bool TaskUpdateChecker::parseBuffer(const QByteArray &buffer)
{
    QString errorString;
    const auto map = JsonUtil::jsonToVariant(buffer, errorString).toMap();
    if (!errorString.isEmpty()) {
        qCWarning(LC) << "JSON error:" << errorString;
        return false;
    }

    // Version (eg. "v1.4.0")
    const QString tagName = map["tag_name"].toString();
    m_version = tagName.mid(1);
    if (m_version.isEmpty())
        return false;

    // `releases/latest/` cannot contain draft/prerelease

    // Check Assets
    const QVariantList assets = map["assets"].toList();
    if (assets.isEmpty())
        return false;

    m_releaseName = map["name"].toString(); // eg. "Fort Firewall v1.4.0"
    m_publishedAt = map["published_at"].toString(); // eg. "2017-12-17T02:27:19Z"

    m_releaseNotes = map["body"].toString(); // ChangeLog

    // Cut release text from dashes
    const int releaseDashesPos = m_releaseNotes.indexOf("\n---");
    if (releaseDashesPos > 0) {
        m_releaseNotes.truncate(releaseDashesPos);
    }

    m_releaseNotes = m_releaseNotes.toHtmlEscaped();

    // Assets
    const QVariantMap assetMap = findDownloadableAsset(assets);

    // eg. "https://github.com/tnodir/fort/releases/download/v1.4.0/FortFirewall-1.4.0.exe"
    m_downloadUrl = assetMap["browser_download_url"].toString();
    m_downloadSize = assetMap["size"].toInt();

    return !m_downloadUrl.isEmpty() && m_downloadSize != 0;
}
