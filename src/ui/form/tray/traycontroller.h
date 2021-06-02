#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class HotKeyManager;
class IniOptions;
class IniUser;
class TranslationManager;

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
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();
};

#endif // TRAYCONTROLLER_H
