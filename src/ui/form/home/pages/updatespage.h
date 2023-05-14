#ifndef UPDATESPAGE_H
#define UPDATESPAGE_H

#include "homebasepage.h"

class UpdatesPage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit UpdatesPage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
};

#endif // UPDATESPAGE_H
