#include "aboutpage.h"

#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <util/iconcache.h>

AboutPage::AboutPage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void AboutPage::onRetranslateUi()
{
}

void AboutPage::setupUi()
{
    auto layout = new QVBoxLayout();

    this->setLayout(layout);
}
