#ifndef APPPARSEOPTIONS_H
#define APPPARSEOPTIONS_H

#include <QMap>
#include <QObject>
#include <QVarLengthArray>

#include <common/fortconf.h>

#include "addressrange.h"

using addrranges_arr_t = QVarLengthArray<AddressRange, 2>;
using appdata_map_t = QMap<QString, FORT_APP_DATA>;

class AppParseOptions
{
public:
    appdata_map_t &appsMap(bool isWild, bool isPrefix);
    quint32 &appsSize(bool isWild, bool isPrefix);

public:
    bool procWild = false;

    quint32 wildAppsSize = 0;
    quint32 prefixAppsSize = 0;
    quint32 exeAppsSize = 0;

    appdata_map_t wildAppsMap;
    appdata_map_t prefixAppsMap;
    appdata_map_t exeAppsMap;
};

#endif // APPPARSEOPTIONS_H
