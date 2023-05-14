#ifndef ABOUTPAGE_H
#define ABOUTPAGE_H

#include "homebasepage.h"

class AboutPage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit AboutPage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupNewVersionBox();
    void setupNewVersionUpdate();

private:
    QGroupBox *m_gbNewVersion = nullptr;
    QLabel *m_labelNewVersion = nullptr;
    QPushButton *m_btNewVersion = nullptr;
};

#endif // ABOUTPAGE_H
