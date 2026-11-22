#include "windowshow.h"

#include <QStringList>

const WindowShow *WindowShow::byName(const QString &name)
{
    for (const auto &windowShow : windowShows) {
        if (name == QLatin1String(windowShow.name))
            return &windowShow;
    }

    return nullptr;
}

QString WindowShow::namesText()
{
    QStringList names;

    for (const auto &windowShow : windowShows) {
        names.append(windowShow.name);
    }

    return names.join('|');
}
