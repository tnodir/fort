#ifndef FORTGLOBAL_H
#define FORTGLOBAL_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortSettings;
class IniOptions;

namespace Fort {

ConfManager *confManager();
FirewallConf *conf();
FortSettings *settings();
IniOptions &iniOpt();

}

#endif // FORTGLOBAL_H
