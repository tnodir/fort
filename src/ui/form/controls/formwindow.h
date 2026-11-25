#ifndef FORMWINDOW_H
#define FORMWINDOW_H

#include <form/form_types.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class HeaderButtons;
class IniUser;
class WidgetWindowStateWatcher;

class FormWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit FormWindow(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    virtual WindowCode windowCode() const { return WindowNone; }
    virtual QString windowIconPath() const { return {}; }
    virtual QString helpSection() const { return {}; }
    virtual QString iniGroup() const { return {}; }

    virtual bool isDockable() const { return (windowCode() & WindowDockable) != 0; }
    bool isDocked() const { return !isWindow(); }

    // The window is to be created anew for another host
    virtual bool isHostChanged() const { return false; }

    virtual bool deleteOnClose() const { return true; }

    bool excludeFromCapture() const { return m_excludeFromCapture; }
    void setExcludeFromCapture(bool v);

    WidgetWindowStateWatcher *stateWatcher() const { return m_stateWatcher; }

    // The dock's title bar shows the header of a docked window
    QWidget *takeHeaderWidget();
    void restoreHeaderWidget();

    // F1 opens the helpSection()
    QAction *helpAction() const { return m_helpAction; }

    void shrinkToContents();
    void shrinkToContentsLater();

    virtual void saveWindowState(bool wasVisible) { Q_UNUSED(wasVisible); }
    virtual void restoreWindowState() { }

    virtual void onWindowClosed() { }

signals:
    void aboutToDelete();

protected:
    // The capture option is of the captureIniGroup, by default of the window's ini group
    void setupFormWindow(IniUser &iniUser, const QString &captureIniGroup = {});

    QWidget *createHeaderWidget(QLayout *headerLayout);

    void saveIniWindowGeometry();
    void restoreIniWindowGeometry(const QSize &defaultSize);

    void changeEvent(QEvent *event) override;

private:
    void setupWindowIcon(IniUser &iniUser);

    void setupActionShortcuts();
    void setupWindowMenuButton();

    void updateDocked();
    void updateDockedMinimumSize(bool docked);

    void setupStateWatcher();
    void setupWindowCapture(IniUser &iniUser, const QString &iniGroup);

private:
    bool m_excludeFromCapture : 1 = false;

    int m_headerIndex = -1;

    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
    QWidget *m_headerWidget = nullptr;
    HeaderButtons *m_headerButtons = nullptr;
    QAction *m_helpAction = nullptr;

    QSize m_windowMinimumSize;
};

#endif // FORMWINDOW_H
