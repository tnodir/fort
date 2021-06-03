#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class IniOptions;
class IniUser;
class TranslationManager;
class WindowManager;

class StatisticsController : public QObject
{
    Q_OBJECT

public:
    explicit StatisticsController(QObject *parent = nullptr);

    FortManager *fortManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);

    void retranslateUi();
};

#endif // STATISTICSCONTROLLER_H
