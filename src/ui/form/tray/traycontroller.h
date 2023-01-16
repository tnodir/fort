#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>

class ConfManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings;
class HotKeyManager;
class IniOptions;
class IniUser;
class TranslationManager;
class WindowManager;

class TrayController : public QObject
{
    Q_OBJECT

public:
    explicit TrayController(QObject *parent = nullptr);

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

signals:
    void retranslateUi();
};

#endif // TRAYCONTROLLER_H
