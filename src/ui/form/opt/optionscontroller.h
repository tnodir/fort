#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <conf/firewallconf.h>
#include <conf/inioptions.h>
#include <form/basecontroller.h>
#include <user/iniuser.h>

class ZoneListModel;

class OptionsController : public BaseController
{
    Q_OBJECT

public:
    explicit OptionsController(QObject *parent = nullptr);

    FirewallConf *confToEdit() { return &m_confToEdit; }
    const FirewallConf *confToEdit() const { return &m_confToEdit; }

    IniOptions &iniOptToEdit() { return m_iniOptToEdit; }
    const IniOptions &iniOptToEdit() const { return m_iniOptToEdit; }

    IniUser &iniUserToEdit() { return m_iniUserToEdit; }
    const IniUser &iniUserToEdit() const { return m_iniUserToEdit; }

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

    void initConfToEdit();
    void resetConfToEdit();

private:
    bool m_iniUserEdited : 1 = false;
    bool m_iniUserDataChanged : 1 = false;
    bool m_iniUserFlagsChanged : 1 = false;

    FirewallConf m_confToEdit;

    IniOptions m_iniOptToEdit;
    IniUser m_iniUserToEdit;
};

#endif // OPTIONSCONTROLLER_H
