#ifndef HOMEMAINPAGE_H
#define HOMEMAINPAGE_H

#include "homebasepage.h"

QT_FORWARD_DECLARE_CLASS(QStackedLayout)

class HomeMainPage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit HomeMainPage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupSideBar();
    void setupSideBarButtons();
    void setupStackedLayout();

    void setCurrentIndex(int tabIndex);

private:
    QToolButton *m_btUpdates = nullptr;
    QToolButton *m_btAbout = nullptr;
    QStackedLayout *m_stackedLayout = nullptr;
};

#endif // HOMEMAINPAGE_H
