#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <form/basecontroller.h>

class IniUser;
class ZoneListModel;

class OptionsController : public BaseController
{
    Q_OBJECT

public:
    explicit OptionsController(QObject *parent = nullptr);
    ~OptionsController() override;

    FirewallConf *confToEdit() const;
    IniUser *iniUserToEdit() const;

    bool anyEdited() const;

signals:
    void editedChanged(bool anyEdited);

    void aboutToSave();
    void editResetted();
    void resetToDefault();

    void afterSaveWindowState(IniUser &ini);
    void afterRestoreWindowState(IniUser &ini);

public slots:
    void setOptEdited();
    void setFlagsEdited();
    void setIniEdited();
    void setTaskEdited();

    void setIniUserEdited(bool flagsChanged = false);

    void emitEdited(bool edited = true);
    void resetEdited();

    void saveChanges() { save(/*closeOnSuccess=*/true); }
    void applyChanges() { save(/*closeOnSuccess=*/false); }

    void exportBackup();
    void importBackup(bool onlyNewApps = false);
    void confirmImportBackup();
    void confirmImportAppsBackup();

    void closeWindow();

private:
    void save(bool closeOnSuccess);
    void saveIniUser(bool onlyFlags);

    void initConfManagerToEdit();
    void closeConfManagerToEdit();

private:
    bool m_iniUserEdited : 1 = false;
    bool m_iniUserDataChanged : 1 = false;
    bool m_iniUserFlagsChanged : 1 = false;
};

#endif // OPTIONSCONTROLLER_H
