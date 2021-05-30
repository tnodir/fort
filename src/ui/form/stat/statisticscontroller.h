#ifndef STATISTICSCONTROLLER_H
#define STATISTICSCONTROLLER_H

#include <QObject>

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class StatManager;
class TranslationManager;

class StatisticsController : public QObject
{
    Q_OBJECT

public:
    explicit StatisticsController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    StatManager *statManager() const;
    TranslationManager *translationManager() const;

signals:
    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);

    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // STATISTICSCONTROLLER_H
