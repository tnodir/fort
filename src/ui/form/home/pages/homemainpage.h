#ifndef HOMEMAINPAGE_H
#define HOMEMAINPAGE_H

#include "homebasepage.h"

QT_FORWARD_DECLARE_CLASS(QStackedWidget)

class HomeMainPage : public HomeBasePage
{
    Q_OBJECT

public:
    enum TabIndex : qint8 { TabHome = 0, TabAbout };
    Q_ENUM(TabIndex)

    explicit HomeMainPage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

    void setCurrentTab(TabIndex tabIndex);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupSideBar();
    void setupSideBarButtons();
    void setupStackedLayout();

    QToolButton *buttonAt(TabIndex tabIndex) const { return m_buttons[tabIndex]; }

private:
    QVector<QToolButton *> m_buttons;
    QStackedWidget *m_stackedPages = nullptr;
};

#endif // HOMEMAINPAGE_H
