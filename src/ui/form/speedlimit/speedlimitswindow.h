#ifndef SPEEDLIMITSWINDOW_H
#define SPEEDLIMITSWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

class SpeedLimitEditDialog;
class SpeedLimitsController;
class TableView;

struct SpeedLimitRow;

class SpeedLimitsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit SpeedLimitsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowSpeedLimits; }
    QString windowOverlayIconPath() const override { return ":/icons/speedometer.png"; }

    SpeedLimitsController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupTableSpeedLimits();
    void setupTableSpeedLimitsHeader();
    void setupTableSpeedLimitsChanged();
    void setupSpeedLimitListModelChanged();
    void setupStatusTimer();

    void updateStatusTimer();

    void addNewSpeedLimit();
    void editSelectedSpeedLimit();

    void openSpeedLimitEditForm(const SpeedLimitRow &speedLimitRow);

    void deleteSpeedLimit(int row);
    void deleteSelectedSpeedLimit();

    int speedLimitListCurrentIndex() const;

private:
    SpeedLimitsController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddSpeedLimit = nullptr;
    QAction *m_actEditSpeedLimit = nullptr;
    QAction *m_actRemoveSpeedLimit = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_speedLimitListView = nullptr;

    QTimer *m_statusTimer = nullptr;

    SpeedLimitEditDialog *m_formSpeedLimitEdit = nullptr;
};

#endif // SPEEDLIMITSWINDOW_H
