#include "updatespage.h"

#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <util/iconcache.h>

UpdatesPage::UpdatesPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void UpdatesPage::onRetranslateUi()
{
}

void UpdatesPage::setupUi()
{
    auto layout = new QVBoxLayout();

    this->setLayout(layout);
}
