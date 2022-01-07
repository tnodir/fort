#include "servicespage.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <form/opt/optionscontroller.h>
#include <model/servicelistmodel.h>
#include <serviceinfo/serviceinfomanager.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

ServicesPage::ServicesPage(OptionsController *ctrl, QWidget *parent) :
    OptBasePage(ctrl, parent), m_serviceListModel(new ServiceListModel(this))
{
    setupUi();
}

ServiceInfoManager *ServicesPage::serviceInfoManager() const
{
    return IoC<ServiceInfoManager>();
}

void ServicesPage::onPageActivated()
{
    serviceListModel()->initialize();
}

void ServicesPage::onRetranslateUi()
{
    m_btRefresh->setText(tr("Refresh"));
    m_btEdit->setText(tr("Edit"));
    m_actEditService->setText(tr("Edit Service"));

    m_btOptions->setText(tr("Options"));

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

    m_btRefresh = ControlUtil::createButton(
            ":/icons/arrow_refresh_small.png", [&] { serviceListModel()->initialize(); });

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

    layout->addWidget(m_btRefresh);
    layout->addWidget(m_btEdit);
    layout->addStretch();
    layout->addWidget(m_btOptions);

    return layout;
}

void ServicesPage::setupOptions()
{
    // Menu
    const QList<QWidget *> menuWidgets = {};
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btOptions = ControlUtil::createButton(":/icons/gear_in.png");
    m_btOptions->setMenu(menu);
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
    header->setSectionResizeMode(3, QHeaderView::Interactive);

    header->resizeSection(0, 240);
    header->resizeSection(1, 290);
    header->resizeSection(2, 80);
    header->resizeSection(3, 90);
}
