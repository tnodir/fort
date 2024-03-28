#include "appparseoptions.h"

appdata_map_t &AppParseOptions::appsMap(bool isWild, bool isPrefix)
{
    return isWild ? wildAppsMap : (isPrefix ? prefixAppsMap : exeAppsMap);
}

quint32 &AppParseOptions::appsSize(bool isWild, bool isPrefix)
{
    return isWild ? wildAppsSize : (isPrefix ? prefixAppsSize : exeAppsSize);
}
