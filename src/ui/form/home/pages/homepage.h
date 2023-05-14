#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include "homebasepage.h"

class HomePage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit HomePage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
};

#endif // HOMEPAGE_H
