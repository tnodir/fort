#ifndef BASECONTROLLER_H
#define BASECONTROLLER_H

#include <QObject>

class ConfManager;
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
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    HotKeyManager *hotKeyManager() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    TaskManager *taskManager() const;

signals:
    void retranslateUi();
};

#endif // BASECONTROLLER_H
