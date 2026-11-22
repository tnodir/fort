#ifndef OPTBASEPAGE_H
#define OPTBASEPAGE_H

#include <QFrame>
#include <QUrl>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QGroupBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QSpinBox)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QTabBar)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class FirewallConf;
class IniOptions;
class IniUser;
class OptionsController;

class OptBasePage : public QFrame
{
    Q_OBJECT

public:
    explicit OptBasePage(OptionsController *ctrl, QWidget *parent = nullptr);

    OptionsController *ctrl() const { return m_ctrl; }
    FirewallConf &conf() const;
    IniOptions &ini() const;
    IniUser &iniUser() const;

    virtual QString helpSection() const { return {}; }

public slots:
    virtual void onPageActivated() { }
    virtual void onResetToDefault() { }

protected:
    // The warning's icon with the label
    static QWidget *createWarning(QLabel *label);

    // The warning by the label, shown only without the Service
    static QWidget *createNoServiceWarning(QLabel *label);

    // The ini option's check box: its toggling sets the option and marks the ini edited
    QCheckBox *createIniCheckBox(
            bool (IniOptions::*iniFlag)() const, void (IniOptions::*setIniFlag)(bool));

    // The callback of the ini option's control: sets the changed option and marks the ini edited
    std::function<void(int)> iniValueSetter(
            int (IniOptions::*iniValue)() const, void (IniOptions::*setIniValue)(int));

    // The user's ini option's check box: its toggling sets the option and marks the user's ini
    // edited, its flags changed by flagsChanged
    QCheckBox *createIniUserCheckBox(bool (IniUser::*iniFlag)() const,
            void (IniUser::*setIniFlag)(bool), bool flagsChanged = false);

    // The callback of the user's ini option's control: sets the changed option and marks
    // the user's ini edited
    std::function<void(int)> iniUserValueSetter(
            int (IniUser::*iniValue)() const, void (IniUser::*setIniValue)(int));

protected slots:
    virtual void onAboutToSave() { }
    virtual void onEditResetted() { }

    virtual void onSaveWindowState(IniUser & /*ini*/) { }
    virtual void onRestoreWindowState(IniUser & /*ini*/) { }

    virtual void onRetranslateUi() { }

private:
    void setupController();

private:
    OptionsController *m_ctrl = nullptr;
};

#endif // OPTBASEPAGE_H
