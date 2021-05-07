#ifndef TRAYCONTROLLER_H
#define TRAYCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class HotKeyManager;
class IniOptions;
class TranslationManager;

class TrayController : public QObject
{
    Q_OBJECT

public:
    explicit TrayController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    HotKeyManager *hotKeyManager() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // TRAYCONTROLLER_H
