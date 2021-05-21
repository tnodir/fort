#include "servicespage.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../model/servicelistmodel.h"
#include "../../../user/iniuser.h"
#include "../../controls/controlutil.h"
#include "../../controls/tableview.h"
#include "../optionscontroller.h"

ServicesPage::ServicesPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void ServicesPage::onRetranslateUi()
{
}

void ServicesPage::setupUi()
{
}

QLayout *ServicesPage::setupHeader()
{
    return nullptr;
}

void ServicesPage::setupOptions()
{
}

void ServicesPage::setupFilterServices()
{
}

void ServicesPage::setupTableServList()
{
}

void ServicesPage::setupTableServListHeader()
{
}
