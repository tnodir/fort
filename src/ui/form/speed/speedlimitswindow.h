#ifndef SPEEDLIMITSWINDOW_H
#define SPEEDLIMITSWINDOW_H

#include <conf/speedlimit.h>
#include <form/controls/formwindow.h>

class ConfManager;
class FirewallConf;
class IniOptions;
class IniUser;
class SpeedLimitEditDialog;
class SpeedLimitListModel;
class SpeedLimitsController;
class TableView;
class WindowManager;

struct SpeedLimitRow;

class SpeedLimitsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit SpeedLimitsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowSpeedLimits; }

    SpeedLimitsController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    FirewallConf *conf() const;
    IniOptions *ini() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    SpeedLimitListModel *speedLimitListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();

private:
    SpeedLimitsController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddSpeedLimit = nullptr;
    QAction *m_actEditSpeedLimit = nullptr;
    QAction *m_actRemoveSpeedLimit = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_speedLimitListView = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    SpeedLimitEditDialog *m_formSpeedLimitEdit = nullptr;
};

#endif // SPEEDLIMITSWINDOW_H
