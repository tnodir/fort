#ifndef FORMPOINTER_H
#define FORMPOINTER_H

#include <QObject>
#include <QPointer>

#include <memory>
#include <optional>

#include <form/controls/formwindow.h>

class PanelDock;

class FormPointer
{
public:
    FormPointer(WindowCode code = WindowNone);

    WindowCode code() const { return m_code; }
    FormWindow *window() const { return m_window.get(); }
    PanelDock *dock() const;

    bool isShown() const;

    FormWindow *initialize();

    bool show(bool activate = true, bool expose = false);
    bool close(bool isAppQuitting = false);

    bool moveToDock();
    bool moveToWindow(const std::optional<QPoint> &pos = std::nullopt);

private:
    bool isIniDocked() const;
    void setIniDocked(bool docked);

    bool showWindow(bool activate, bool expose);
    bool showDocked(bool activate, bool expose);
    void takeFromDock();

private:
    WindowCode m_code = WindowNone;

    std::unique_ptr<FormWindow> m_window;

    QPointer<PanelDock> m_dock;
};

#endif // FORMPOINTER_H
