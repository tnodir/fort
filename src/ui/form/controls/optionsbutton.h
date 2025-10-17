#ifndef OPTIONSBUTTON_H
#define OPTIONSBUTTON_H

#include "toolbutton.h"

class TrayIcon;

class OptionsButton : public ToolButton
{
    Q_OBJECT

public:
    explicit OptionsButton(int tabIndex = 0, QWidget *parent = nullptr);

public slots:
    void showOptionsWindow();

private:
    void setupUi();

private:
    int m_tabIndex = 0;
};

#endif // OPTIONSBUTTON_H
