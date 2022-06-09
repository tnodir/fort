#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <QObject>

class ConfManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class TaskManager;
class TranslationManager;
class WindowManager;
class ZoneListModel;

class OptionsController : public QObject
{
    Q_OBJECT

public:
    explicit OptionsController(QObject *parent = nullptr);
    ~OptionsController() override;

    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    TaskManager *taskManager() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    ZoneListModel *zoneListModel() const;

    void initialize();

signals:
    void editedChanged(bool anyEdited);

    void aboutToSave();
    void editResetted();

    void cancelChanges(IniOptions *oldIni);

    void afterSaveWindowState(IniUser *ini);
    void afterRestoreWindowState(IniUser *ini);

    void retranslateUi();

public slots:
    void setOptEdited();
    void setFlagsEdited();
    void setIniEdited();
    void setTaskEdited();

    void emitEdited(bool edited = true);
    void resetEdited();

    void saveChanges() { save(/*closeOnSuccess=*/true); }
    void applyChanges() { save(/*closeOnSuccess=*/false); }

    void closeWindow();

private:
    void save(bool closeOnSuccess);
};

#endif // OPTIONSCONTROLLER_H
