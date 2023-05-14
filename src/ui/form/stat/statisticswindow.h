#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <util/window/widgetwindow.h>

class ConfManager;
class IniUser;
class StatisticsController;
class WidgetWindowStateWatcher;

class StatisticsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit StatisticsWindow(QWidget *parent = nullptr);

    StatisticsController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;

    void saveWindowState();
    void restoreWindowState();

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();

private:
    StatisticsController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;
};

#endif // STATISTICSWINDOW_H
