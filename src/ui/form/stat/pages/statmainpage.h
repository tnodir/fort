#ifndef STATMAINPAGE_H
#define STATMAINPAGE_H

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class StatMainPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit StatMainPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupTabBar();

private:
    QTabWidget *m_tabBar = nullptr;

    QPushButton *m_btMenu = nullptr;
};

#endif // STATMAINPAGE_H
