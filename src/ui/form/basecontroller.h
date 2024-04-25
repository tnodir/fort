#ifndef BASECONTROLLER_H
#define BASECONTROLLER_H

#include <QObject>

class AutoUpdateManager;
class ConfAppManager;
class ConfManager;
class ConfRuleManager;
class ConfZoneManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings;
class HotKeyManager;
class IniOptions;
class IniUser;
class TaskManager;
class TranslationManager;
class WindowManager;

class BaseController : public QObject
{
    Q_OBJECT

public:
    explicit BaseController(QObject *parent = nullptr);

    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    ConfAppManager *confAppManager() const;
    ConfRuleManager *confRuleManager() const;
    ConfZoneManager *confZoneManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    AutoUpdateManager *autoUpdateManager() const;
    TaskManager *taskManager() const;

signals:
    void retranslateUi();

public slots:
    void onLinkClicked();
};

#endif // BASECONTROLLER_H
