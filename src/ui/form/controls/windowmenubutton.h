#ifndef WINDOWMENUBUTTON_H
#define WINDOWMENUBUTTON_H

#include "toolbutton.h"

class FormWindow;

class WindowMenuButton : public ToolButton
{
    Q_OBJECT

public:
    explicit WindowMenuButton(QWidget *parent = nullptr);

    bool docked() const { return m_docked; }
    void setDocked(bool v);

    FormWindow *formWindow() const { return m_formWindow; }
    void setFormWindow(FormWindow *v) { m_formWindow = v; }

protected:
    void initStyleOption(QStyleOptionToolButton *option) const override;

private:
    void setupUi();
    void setupMenu();
    void retranslateUi();

    void onDockTriggered();

private:
    bool m_docked = false;

    FormWindow *m_formWindow = nullptr;

    QAction *m_actDock = nullptr;
};

#endif // WINDOWMENUBUTTON_H
