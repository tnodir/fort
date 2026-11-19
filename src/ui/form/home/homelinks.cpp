#include "homelinks.h"

#include <fort_version.h>

#include <fortglobal.h>
#include <fortsettings.h>
#include <util/osutil.h>

using namespace Fort;

namespace {

inline constexpr const char *linkTexts[] = {
    QT_TRANSLATE_NOOP("HomeLinks", "Profile"),
    QT_TRANSLATE_NOOP("HomeLinks", "Logs"),
    QT_TRANSLATE_NOOP("HomeLinks", "Service Logs"),
    QT_TRANSLATE_NOOP("HomeLinks", "Releases"),
    QT_TRANSLATE_NOOP("HomeLinks", "Help"),
};

inline constexpr const char *linkIconPaths[] = {
    ":/icons/folder.png",
    ":/icons/folder.png",
    ":/icons/folder.png",
    ":/icons/github.png",
    ":/icons/help.png",
};

static_assert(std::size(linkTexts) == HomeLinks::LinkCount, "Missing a Link in linkTexts[]");
static_assert(
        std::size(linkIconPaths) == HomeLinks::LinkCount, "Missing a Link in linkIconPaths[]");

}

QString HomeLinks::text(Link link)
{
    return tr(linkTexts[link]);
}

QString HomeLinks::iconPath(Link link)
{
    return linkIconPaths[link];
}

QString HomeLinks::path(Link link)
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

bool HomeLinks::isVisible(Link link)
{
    if (link != LinkServiceLogs)
        return true;

    const auto settings = Fort::settings();

    return settings->hasService() && settings->profileLogsPath() != settings->logsPath();
}

void HomeLinks::open(Link link)
{
    OsUtil::openUrlOrFolder(path(link));
}
