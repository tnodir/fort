#ifndef FORMWINDOW_H
#define FORMWINDOW_H

#include <form/form_types.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QActionGroup)
QT_FORWARD_DECLARE_CLASS(QBoxLayout)
QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class IniUser;
class WidgetWindowStateWatcher;

class FormWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit FormWindow(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    virtual WindowCode windowCode() const { return WindowNone; }
    virtual QString windowOverlayIconPath() const { return {}; }

    virtual bool deleteOnClose() const { return true; }

    bool excludeFromCapture() const { return m_excludeFromCapture; }
    void setExcludeFromCapture(bool v);

    WidgetWindowStateWatcher *stateWatcher() const { return m_stateWatcher; }

    virtual void saveWindowState(bool wasVisible) { Q_UNUSED(wasVisible); }
    virtual void restoreWindowState() { }

signals:
    void aboutToDelete();

protected:
    void setupFormWindow(IniUser *iniUser, const QString &iniGroup);

private:
    void setupWindowIcon(IniUser *iniUser);

    void setupStateWatcher();
    void setupWindowCapture(IniUser *iniUser, const QString &iniGroup);

private:
    bool m_excludeFromCapture : 1 = false;

    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
};

#endif // FORMWINDOW_H
