#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"
#include "optionscontroller.h"

QT_FORWARD_DECLARE_CLASS(QAbstractButton)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QStackedLayout)
QT_FORWARD_DECLARE_CLASS(QTabWidget)

QT_FORWARD_DECLARE_CLASS(AddressesPage)
QT_FORWARD_DECLARE_CLASS(ApplicationsPage)
QT_FORWARD_DECLARE_CLASS(OptionsPage)
QT_FORWARD_DECLARE_CLASS(SchedulePage)
QT_FORWARD_DECLARE_CLASS(StatisticsPage)

class OptionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit OptionsWindow(FortManager *fortManager,
                           QWidget *parent = nullptr);

signals:

public slots:
    void retranslateUi();

private slots:
    void onLinkClicked();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    OptionsController *ctrl() { return &m_ctrl; }

    void setupUi();
    void setupPages();
    QLayout *setupDialogButtons();
    void setupNewVersionButton();

    FortSettings *fortSettings();
    TaskManager *taskManager();

    static QAbstractButton *createLinkButton(const QString &iconPath,
                                             const QString &linkPath = QString(),
                                             const QString &toolTip = QString());

private:
    OptionsController m_ctrl;

    QTabWidget *m_tabBar = nullptr;
    QStackedLayout *m_stackLayout = nullptr;

    QAbstractButton *m_logsButton = nullptr;
    QAbstractButton *m_profileButton = nullptr;
    QAbstractButton *m_statButton = nullptr;
    QAbstractButton *m_releasesButton = nullptr;
    QAbstractButton *m_newVersionButton = nullptr;

    QPushButton *m_okButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    OptionsPage *m_optionsPage = nullptr;
    AddressesPage *m_addressesPage = nullptr;
    ApplicationsPage *m_applicationsPage = nullptr;
    StatisticsPage *m_statisticsPage = nullptr;
    SchedulePage *m_schedulePage = nullptr;
};

#endif // OPTIONSWINDOW_H
