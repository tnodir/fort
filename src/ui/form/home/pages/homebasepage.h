#ifndef HOMEBASEPAGE_H
#define HOMEBASEPAGE_H

#include <QFrame>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QProgressBar)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class AutoUpdateManager;
class ConfManager;
class DriverManager;
class FirewallConf;
class FortManager;
class FortSettings ;
class HomeController;
class IniUser;
class TaskManager;
class TranslationManager;
class WindowManager;

class HomeBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit HomeBasePage(HomeController *ctrl, QWidget *parent = nullptr);

protected:
    HomeController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniUser *iniUser() const;
    DriverManager *driverManager() const;
    AutoUpdateManager *autoUpdateManager() const;
    TaskManager *taskManager() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;

protected slots:
    virtual void onSaveWindowState(IniUser * /*ini*/) { }
    virtual void onRestoreWindowState(IniUser * /*ini*/) { }

    virtual void onRetranslateUi() { }

private:
    void setupController();

    void onPasswordLockedChanged();

private:
    HomeController *m_ctrl = nullptr;
};

#endif // HOMEBASEPAGE_H
