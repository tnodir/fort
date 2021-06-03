#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class TaskManager;
class TranslationManager;
class WindowManager;
class ZoneListModel;

class ZonesController : public QObject
{
    Q_OBJECT

public:
    explicit ZonesController(QObject *parent = nullptr);

    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

signals:
    void retranslateUi();
};

#endif // ZONESCONTROLLER_H
