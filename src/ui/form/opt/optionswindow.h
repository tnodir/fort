#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

class ConfManager;
class FortManager;
class IniUser;
class OptMainPage;
class OptionsController;
class WidgetWindowStateWatcher;

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(QWidget *parent = nullptr);

    quint32 windowCode() const override { return WindowOptions; }

    OptionsController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;

    void selectTab(int index);

    void cancelChanges();

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();

    void checkDeprecatedAppGroups();

private:
    OptionsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    OptMainPage *m_mainPage = nullptr;
};

#endif // OPTIONSWINDOW_H
