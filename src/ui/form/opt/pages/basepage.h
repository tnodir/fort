#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QFrame>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTabBar)

class ConfManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class OptionsController;
class TaskManager;
class TranslationManager;
class ZoneListModel;

class BasePage : public QFrame
{
    Q_OBJECT

public:
    explicit BasePage(OptionsController *ctrl, QWidget *parent = nullptr);

protected:
    OptionsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    DriverManager *driverManager() const;
    TranslationManager *translationManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

protected slots:
    virtual void onAboutToSave() { }
    virtual void onEditResetted() { }

    virtual void onCancelChanges(IniOptions * /*oldIni*/) { }

    virtual void onSaveWindowState(IniUser * /*ini*/) { }
    virtual void onRestoreWindowState(IniUser * /*ini*/) { }

    virtual void onRetranslateUi() { }

    void onLinkClicked();

private:
    void setupController();

private:
    OptionsController *m_ctrl = nullptr;
};

#endif // BASEPAGE_H
