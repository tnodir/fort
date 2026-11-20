#include "applinks.h"

#include <fort_version.h>

#include <fortglobal.h>
#include <fortsettings.h>
#include <util/osutil.h>

using namespace Fort;

namespace {

inline constexpr const char *linkTexts[] = {
    QT_TRANSLATE_NOOP("AppLinks", "Profile"),
    QT_TRANSLATE_NOOP("AppLinks", "Logs"),
    QT_TRANSLATE_NOOP("AppLinks", "Service Logs"),
    QT_TRANSLATE_NOOP("AppLinks", "Releases"),
    QT_TRANSLATE_NOOP("AppLinks", "Help"),
};

inline constexpr const char *linkIconPaths[] = {
    ":/icons/folder.png",
    ":/icons/folder.png",
    ":/icons/folder.png",
    ":/icons/github.png",
    ":/icons/help.png",
};

static_assert(std::size(linkTexts) == AppLinks::LinkCount, "Missing a Link in linkTexts[]");
static_assert(std::size(linkIconPaths) == AppLinks::LinkCount, "Missing a Link in linkIconPaths[]");

}

QString AppLinks::text(Link link)
{
    return tr(linkTexts[link]);
}

QString AppLinks::iconPath(Link link)
{
    return linkIconPaths[link];
}

QString AppLinks::path(Link link)
{
    const auto settings = Fort::settings();

    switch (link) {
    case LinkProfile:
        return settings->profilePath();
    case LinkLogs:
        return settings->logsPath();
    case LinkServiceLogs:
        return settings->profileLogsPath();
    case LinkReleases:
        return APP_UPDATES_URL;
    case LinkHelp:
        return APP_WIKI_URL;
    default:
        return {};
    }
}

bool AppLinks::isVisible(Link link)
{
    if (link != LinkServiceLogs)
        return true;

    const auto settings = Fort::settings();

    return settings->hasService() && settings->profileLogsPath() != settings->logsPath();
}

void AppLinks::open(Link link)
{
    OsUtil::openUrlOrFolder(path(link));
}
