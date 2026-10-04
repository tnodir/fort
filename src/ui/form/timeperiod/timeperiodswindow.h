#ifndef TIMEPERIODSWINDOW_H
#define TIMEPERIODSWINDOW_H

#include <form/controls/formwindow.h>

class TableView;
class TimePeriodEditDialog;
class TimePeriodsController;

struct TimePeriodRow;

class TimePeriodsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit TimePeriodsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowTimePeriods; }
    QString windowOverlayIconPath() const override { return ":/icons/clock.png"; }

    TimePeriodsController *ctrl() const { return m_ctrl; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupTableTimePeriods();
    void setupTableTimePeriodsHeader();
    void setupTableTimePeriodsChanged();
    void setupTimePeriodListModelChanged();

    void addNewTimePeriod();
    void editSelectedTimePeriod();

    void openTimePeriodEditForm(const TimePeriodRow &timePeriodRow);

    void deleteTimePeriod(int row);
    void deleteSelectedTimePeriod();

    int timePeriodListCurrentIndex() const;

private:
    TimePeriodsController *m_ctrl = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddTimePeriod = nullptr;
    QAction *m_actEditTimePeriod = nullptr;
    QAction *m_actRemoveTimePeriod = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_timePeriodListView = nullptr;

    TimePeriodEditDialog *m_formTimePeriodEdit = nullptr;
};

#endif // TIMEPERIODSWINDOW_H
