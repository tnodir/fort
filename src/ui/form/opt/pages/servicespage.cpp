#include "servicespage.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../../conf/firewallconf.h"
#include "../../../model/servicelistmodel.h"
#include "../../../serviceinfo/serviceinfomanager.h"
#include "../../../user/iniuser.h"
#include "../../../util/iconcache.h"
#include "../../../util/ioc/ioccontainer.h"
#include "../../controls/controlutil.h"
#include "../../controls/tableview.h"
#include "../optionscontroller.h"

ServicesPage::ServicesPage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent), m_serviceListModel(new ServiceListModel(this))
{
    setupUi();

    serviceListModel()->initialize();
}

ServiceInfoManager *ServicesPage::serviceInfoManager() const
{
    return IoC<ServiceInfoManager>();
}

void ServicesPage::onRetranslateUi()
{
    m_btEdit->setText(tr("Edit"));
    m_actEditService->setText(tr("Edit Service"));

    m_btOptions->setText(tr("Options"));
    m_cbFilterServices->setText(tr("Filter Services"));

    serviceListModel()->refresh();
}

void ServicesPage::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableServiceList();
    setupTableServiceListHeader();
    layout->addWidget(m_serviceListView, 1);

    this->setLayout(layout);
}

QLayout *ServicesPage::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Edit Menu
    auto editMenu = new QMenu(this);

    m_actEditService = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditService->setShortcut(Qt::Key_Return);

    connect(m_actEditService, &QAction::triggered, this, [&] {
        // const auto connIndex = serviceListCurrentIndex();
        // const auto connRow = serviceListModel()->connRowAt(connIndex);

        // showServiceEditForm(connRow.appPath);
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setMenu(editMenu);

    // Options
    setupOptions();

    layout->addWidget(m_btEdit);
    layout->addStretch();
    layout->addWidget(m_btOptions);

    return layout;
}

void ServicesPage::setupOptions()
{
    setupFilterServices();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbFilterServices };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/wrench.png");
    m_btOptions->setMenu(menu);
}

void ServicesPage::setupFilterServices()
{
    m_cbFilterServices = ControlUtil::createCheckBox(conf()->filterServices(), [&](bool checked) {
        if (conf()->filterServices() == checked)
            return;

        conf()->setFilterServices(checked);
        ctrl()->setFlagsEdited();

        updateFilterServices();
    });
}

void ServicesPage::setupTableServiceList()
{
    m_serviceListView = new TableView();
    m_serviceListView->setAlternatingRowColors(true);
    m_serviceListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_serviceListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_serviceListView->setModel(serviceListModel());

    m_serviceListView->setMenu(m_btEdit->menu());
}

void ServicesPage::setupTableServiceListHeader()
{
    auto header = m_serviceListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);

    header->resizeSection(0, 250);
    header->resizeSection(1, 350);
    header->resizeSection(2, 90);
}

void ServicesPage::updateFilterServices()
{
    if (conf()->filterServices()) {
        serviceInfoManager()->setEnabled(true);
    }
}
