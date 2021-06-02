#ifndef PROGRAMSCONTROLLER_H
#define PROGRAMSCONTROLLER_H

#include <QObject>

class AppListModel;
class ConfManager;
class FirewallConf;
class FortManager;
class IniOptions;
class IniUser;
class TranslationManager;

class ProgramsController : public QObject
{
    Q_OBJECT

public:
    explicit ProgramsController(QObject *parent = nullptr);

    FortManager *fortManager() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    AppListModel *appListModel() const;
    TranslationManager *translationManager() const;

signals:
    void retranslateUi();
};

#endif // PROGRAMSCONTROLLER_H
