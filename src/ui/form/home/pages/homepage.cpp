#include "homepage.h"

#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/home/homecontroller.h>
#include <util/iconcache.h>

HomePage::HomePage(HomeController *ctrl, QWidget *parent) : HomeBasePage(ctrl, parent)
{
    setupUi();
}

void HomePage::onRetranslateUi()
{
}

void HomePage::setupUi()
{
    auto layout = new QVBoxLayout();

    this->setLayout(layout);
}
