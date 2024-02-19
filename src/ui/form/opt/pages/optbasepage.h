#ifndef OPTBASEPAGE_H
#define OPTBASEPAGE_H

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
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTabBar)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfManager;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class OptionsController;
class TaskManager;
class TranslationManager;
class WindowManager;
class ZoneListModel;

class OptBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit OptBasePage(OptionsController *ctrl, QWidget *parent = nullptr);

    OptionsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    TranslationManager *translationManager() const;
    WindowManager *windowManager() const;
    TaskManager *taskManager() const;
    ZoneListModel *zoneListModel() const;

public slots:
    virtual void onPageActivated() { }
    virtual void onResetToDefault() { }

protected slots:
    virtual void onAboutToSave() { }
    virtual void onEditResetted() { }

    virtual void onSaveWindowState(IniUser * /*ini*/) { }
    virtual void onRestoreWindowState(IniUser * /*ini*/) { }

    virtual void onRetranslateUi() { }

private:
    void setupController();

private:
    OptionsController *m_ctrl = nullptr;
};

#endif // OPTBASEPAGE_H
