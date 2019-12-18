#ifndef OPTIONSWINDOW_H
#define OPTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"
#include "optionscontroller.h"

QT_FORWARD_DECLARE_CLASS(QAbstractButton)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QStackedLayout)
QT_FORWARD_DECLARE_CLASS(QTabBar)

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
    QLayout *setupDialogButtons();
    void setupNewVersionButton();

    FortSettings *fortSettings();
    TaskManager *taskManager();

    static QAbstractButton *createLinkButton(const QString &iconPath,
                                             const QString &linkPath = QString(),
                                             const QString &toolTip = QString());

private:
    OptionsController m_ctrl;

    QTabBar *m_tabBar = nullptr;
    QStackedLayout *m_stackLayout = nullptr;

    QAbstractButton *m_logsButton = nullptr;
    QAbstractButton *m_profileButton = nullptr;
    QAbstractButton *m_statButton = nullptr;
    QAbstractButton *m_releasesButton = nullptr;
    QAbstractButton *m_newVersionButton = nullptr;

    QPushButton *m_okButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
};

#endif // OPTIONSWINDOW_H
