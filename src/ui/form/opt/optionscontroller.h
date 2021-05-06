#ifndef OPTIONSCONTROLLER_H
#define OPTIONSCONTROLLER_H

#include <QObject>

class ConfManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings;
class TaskManager;
class TranslationManager;
class ZoneListModel;

class OptionsController : public QObject
{
    Q_OBJECT

public:
    explicit OptionsController(FortManager *fortManager, QObject *parent = nullptr);
    ~OptionsController() override;

    FortManager *fortManager() const { return m_fortManager; }
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    TaskManager *taskManager() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    ZoneListModel *zoneListModel() const;

    void setOthersEdited();
    void setExtEdited();
    void setIniEdited();
    void setFlagsEdited();
    void setOptEdited();

    void resetEdited();

    void initialize();

signals:
    void editedChanged(bool anyEdited);
    void editResetted();

    void saved();

    void afterSaveWindowState();
    void afterRestoreWindowState();

    void retranslateUi();

public slots:
    void closeWindow();

    void saveChanges() { save(true); }
    void applyChanges() { save(false); }

private:
    void save(bool closeOnSuccess);

private:
    FortManager *m_fortManager = nullptr;
};

#endif // OPTIONSCONTROLLER_H
