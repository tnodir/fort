#ifndef PROGRAMSCONTROLLER_H
#define PROGRAMSCONTROLLER_H

#include <QObject>

class AppListModel;
class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class TranslationManager;

class ProgramsController : public QObject
{
    Q_OBJECT

public:
    explicit ProgramsController(FortManager *fortManager, QObject *parent = nullptr);

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    AppListModel *appListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();

private:
    FortManager *m_fortManager = nullptr;
};

#endif // PROGRAMSCONTROLLER_H
