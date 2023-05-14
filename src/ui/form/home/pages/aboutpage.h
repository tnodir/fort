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
};

#endif // ABOUTPAGE_H
