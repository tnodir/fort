#ifndef STATISTICSWINDOW_H
#define STATISTICSWINDOW_H

#include <form/controls/formwindow.h>

class ConfManager;
class IniUser;
class StatisticsController;

class StatisticsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit StatisticsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowStatistics; }

    StatisticsController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();

private:
    StatisticsController *m_ctrl = nullptr;
};

#endif // STATISTICSWINDOW_H
