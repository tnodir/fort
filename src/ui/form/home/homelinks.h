#ifndef HOMELINKS_H
#define HOMELINKS_H

#include <QCoreApplication>

class HomeLinks
{
    Q_DECLARE_TR_FUNCTIONS(HomeLinks)

public:
    enum Link : qint8 {
        LinkProfile = 0,
        LinkLogs,
        LinkServiceLogs,
        LinkReleases,
        LinkHelp,
        LinkCount
    };

    static QString text(Link link);
    static QString iconPath(Link link);

    // The folder's or URL's
    static QString path(Link link);

    static bool isVisible(Link link);

    static void open(Link link);
};

#endif // HOMELINKS_H
