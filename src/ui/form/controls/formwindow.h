#ifndef FORMWINDOW_H
#define FORMWINDOW_H

#include <form/form_types.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDateTimeEdit)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
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

    bool deleteOnClose() const override { return m_deleteOnClose; }
    void setDeleteOnClose(bool v) { m_deleteOnClose = v; }

    bool excludeFromCapture() const { return m_excludeFromCapture; }
    void setExcludeFromCapture(bool v);

    WidgetWindowStateWatcher *stateWatcher() const { return m_stateWatcher; }

    virtual void saveWindowState(bool wasVisible) { Q_UNUSED(wasVisible); }
    virtual void restoreWindowState() { }

protected:
    void setupFormWindow(IniUser *iniUser, const QString &iniGroup);

private:
    void setupStateWatcher();
    void setupWindowCapture(IniUser *iniUser, const QString &iniGroup);

private:
    bool m_deleteOnClose : 1 = false;
    bool m_excludeFromCapture : 1 = false;

    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
};

#endif // FORMWINDOW_H
