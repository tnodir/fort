#include "connectionswindow.h"

#include <QCheckBox>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../conf/firewallconf.h"
#include "../../fortmanager.h"
#include "../../fortsettings.h"
#include "../../model/connlistmodel.h"
#include "../../util/app/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/iconcache.h"
#include "../controls/appinforow.h"
#include "../controls/controlutil.h"
#include "../controls/tableview.h"
#include "connectionscontroller.h"

namespace {

#define CONN_LIST_HEADER_VERSION 1

}

ConnectionsWindow::ConnectionsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent), m_ctrl(new ConnectionsController(fortManager, this))
{
    setupUi();
    setupController();
}

void ConnectionsWindow::setupController()
{
    connect(ctrl(), &ConnectionsController::retranslateUi, this,
            &ConnectionsWindow::onRetranslateUi);

    connect(this, &ConnectionsWindow::aboutToClose, fortManager(),
            &FortManager::closeConnectionsWindow);

    connect(fortManager(), &FortManager::afterSaveConnWindowState, this,
            &ConnectionsWindow::onSaveWindowState);
    connect(fortManager(), &FortManager::afterRestoreConnWindowState, this,
            &ConnectionsWindow::onRestoreWindowState);

    emit ctrl()->retranslateUi();
}

void ConnectionsWindow::onSaveWindowState()
{
    auto header = m_connListView->horizontalHeader();
    settings()->setConnListHeader(header->saveState());
    settings()->setConnListHeaderVersion(CONN_LIST_HEADER_VERSION);
}

void ConnectionsWindow::onRestoreWindowState()
{
    if (settings()->connListHeaderVersion() != CONN_LIST_HEADER_VERSION)
        return;

    auto header = m_connListView->horizontalHeader();
    header->restoreState(settings()->connListHeader());
}

void ConnectionsWindow::onRetranslateUi()
{
    this->unsetLocale();

    m_btLogOptions->setText(tr("Options"));
    m_cbLogAllowedIp->setText(tr("Collect allowed connections"));
    m_cbLogBlockedIp->setText(tr("Collect blocked connections"));

    connListModel()->refresh();

    this->setWindowTitle(tr("Connections"));
}

void ConnectionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(6, 6, 6, 6);

    // Header
    auto header = setupHeader();
    layout->addLayout(header);

    // Table
    setupTableConnList();
    setupTableConnListHeader();
    layout->addWidget(m_connListView, 1);

    // App Info Row
    setupAppInfoRow();
    layout->addWidget(m_appInfoRow);

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png", ":/icons/connect.png"));

    // Size
    this->resize(1300, 768);
    this->setMinimumSize(500, 400);
}

QLayout *ConnectionsWindow::setupHeader()
{
    auto layout = new QHBoxLayout();

    // Log Options
    setupLogOptions();

    layout->addStretch();
    layout->addWidget(m_btLogOptions);

    return layout;
}

void ConnectionsWindow::setupLogOptions()
{
    setupLogAllowedIp();
    setupLogBlockedIp();

    // Menu
    const QList<QWidget *> menuWidgets = { m_cbLogAllowedIp, m_cbLogBlockedIp };
    auto layout = ControlUtil::createLayoutByWidgets(menuWidgets);

    auto menu = ControlUtil::createMenuByLayout(layout, this);

    m_btLogOptions = ControlUtil::createButton(":/icons/wrench.png");
    m_btLogOptions->setMenu(menu);
}

void ConnectionsWindow::setupLogAllowedIp()
{
    m_cbLogAllowedIp = ControlUtil::createCheckBox(conf()->logAllowedIp(), [&](bool checked) {
        if (conf()->logAllowedIp() == checked)
            return;

        conf()->setLogAllowedIp(checked);

        fortManager()->applyConfImmediateFlags();
    });
}

void ConnectionsWindow::setupLogBlockedIp()
{
    m_cbLogBlockedIp = ControlUtil::createCheckBox(conf()->logBlockedIp(), [&](bool checked) {
        if (conf()->logBlockedIp() == checked)
            return;

        conf()->setLogBlockedIp(checked);

        fortManager()->applyConfImmediateFlags();
    });
}

void ConnectionsWindow::setupTableConnList()
{
    m_connListView = new TableView();
    m_connListView->setAlternatingRowColors(true);
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    // m_connListView->setSortingEnabled(true);
    m_connListView->setModel(connListModel());
}

void ConnectionsWindow::setupTableConnListHeader()
{
    auto header = m_connListView->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    header->setSectionResizeMode(4, QHeaderView::Interactive);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    header->setSectionResizeMode(6, QHeaderView::Stretch);

    header->resizeSection(0, 600);
    header->resizeSection(1, 80);
    header->resizeSection(2, 80);
    header->resizeSection(3, 180);
    header->resizeSection(4, 180);
    header->resizeSection(5, 80);

    // header->setSectionsClickable(true);
    // header->setSortIndicatorShown(true);
    // header->setSortIndicator(4, Qt::DescendingOrder);
}

void ConnectionsWindow::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

int ConnectionsWindow::connListCurrentIndex() const
{
    return m_connListView->currentIndex().row();
}

QString ConnectionsWindow::connListCurrentPath() const
{
    const auto connRow = connListModel()->connRowAt(connListCurrentIndex());
    return connRow.appPath;
}

FortManager *ConnectionsWindow::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *ConnectionsWindow::settings() const
{
    return ctrl()->settings();
}

FirewallConf *ConnectionsWindow::conf() const
{
    return ctrl()->conf();
}

ConnListModel *ConnectionsWindow::connListModel() const
{
    return fortManager()->connListModel();
}

AppInfoCache *ConnectionsWindow::appInfoCache() const
{
    return connListModel()->appInfoCache();
}
