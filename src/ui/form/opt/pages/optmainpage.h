#ifndef OPTMAINPAGE_H
#define OPTMAINPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class OptMainPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit OptMainPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupTabBar();
    QLayout *setupDialogButtons();
    void setupApplyCancelButtons();

private:
    QTabWidget *m_tabBar = nullptr;

    QPushButton *m_btLogs = nullptr;
    QPushButton *m_btProfile = nullptr;
    QPushButton *m_btStat = nullptr;
    QPushButton *m_btReleases = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btApply = nullptr;
    QPushButton *m_btCancel = nullptr;

    QVector<OptBasePage *> m_pages;
};

#endif // OPTMAINPAGE_H
