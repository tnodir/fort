#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class IniOptions;
class IniUser;
class TaskManager;
class TranslationManager;
class ZoneListModel;

class ZonesController : public QObject
{
    Q_OBJECT

public:
    explicit ZonesController(QObject *parent = nullptr);

    FortManager *fortManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();
};

#endif // ZONESCONTROLLER_H
