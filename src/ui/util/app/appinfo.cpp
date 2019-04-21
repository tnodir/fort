#include "appinfo.h"

#include "appiconprovider.h"

QString AppInfo::iconPath() const
{
    return AppIconProvider::iconPath(iconId);
}
