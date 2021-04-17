#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"

class FortManager;
class FortSettings;
class MainPage;
class OptionsController;
class WidgetWindowStateWatcher;

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(FortManager *fortManager, QWidget *parent = nullptr);

    OptionsController *ctrl() const { return m_ctrl; }
    FortSettings *settings() const;

    void saveWindowState();
    void restoreWindowState();

protected slots:
    void onRetranslateUi();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupController();
    void setupStateWatcher();

    void setupUi();

private:
    OptionsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    MainPage *m_mainPage = nullptr;
};

#endif // OPTIONSWINDOW_H
