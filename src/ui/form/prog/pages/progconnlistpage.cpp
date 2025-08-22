#include "progconnlistpage.h"

#include <QHeaderView>
#include <QMenu>
#include <QVBoxLayout>

#include <appinfo/appinfocache.h>
#include <conf/app.h>
#include <form/controls/appinforow.h>
#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <model/appconnlistmodel.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/osutil.h>

ProgConnListPage::ProgConnListPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent), m_appConnListModel(new AppConnListModel(this))
{
    setupUi();
}

void ProgConnListPage::onPageInitialize(const App &app)
{
    setupConnListModel(app);
}

void ProgConnListPage::onRetranslateUi()
{
    m_actCopyAsFilter->setText(tr("Copy as Filter"));
    m_actCopy->setText(tr("Copy"));
    m_actLookupIp->setText(tr("Lookup IP"));

    m_appInfoRow->retranslateUi();
}

void ProgConnListPage::setupConnListModel(const App &app)
{
    appConnListModel()->setConfAppId(app.appId);
    if (!app.isWildcard) {
        appConnListModel()->setAppPath(app.appPath);
    }
    appConnListModel()->setResolveAddress(true);
    appConnListModel()->initialize();
}

void ProgConnListPage::setupUi()
{
    setupConnListView();
    setupConnListHeader();
    setupAppInfoRow();

    setupTableConnListChanged();

    // Main Layout
    auto layout =
            ControlUtil::createVLayoutByWidgets({ m_connListView, m_appInfoRow }, /*margin=*/4);

    this->setLayout(layout);
}

void ProgConnListPage::setupConnListView()
{
    m_connListView = new TableView();
    m_connListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_connListView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_connListView->setModel(appConnListModel());

    setupConnListMenu();
}

void ProgConnListPage::setupConnListMenu()
{
    auto menu = ControlUtil::createMenu(m_connListView);

    m_actCopyAsFilter = menu->addAction(IconCache::icon(":/icons/script.png"), QString());
    m_actCopyAsFilter->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_C);

    m_actCopy = menu->addAction(IconCache::icon(":/icons/page_copy.png"), QString());
    m_actCopy->setShortcut(Qt::Key_Copy);

    m_actLookupIp = menu->addAction(IconCache::icon(":/icons/magnifier.png"), QString());
    m_actLookupIp->setShortcut(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_L);

    connect(m_actCopyAsFilter, &QAction::triggered, this, [&] {
        const auto rows = m_connListView->selectedRows();
        const auto text = appConnListModel()->rowsAsFilter(rows);

        GuiUtil::setClipboardData(text);
    });
    connect(m_actCopy, &QAction::triggered, m_connListView, &TableView::copySelectedText);
    connect(m_actLookupIp, &QAction::triggered, this, [&] {
        const auto row = m_connListView->currentRow();
        const auto index = appConnListModel()->index(row, int(ConnListColumn::RemoteIp));
        const auto textIp = appConnListModel()->data(index).toString();

        OsUtil::openIpLocationUrl(textIp);
    });

    m_connListView->setMenu(menu);

    m_connListView->addAction(m_actCopyAsFilter);
    m_connListView->addAction(m_actLookupIp);
}

void ProgConnListPage::setupConnListHeader()
{
    auto header = m_connListView->horizontalHeader();

    header->setSectionResizeMode(int(ConnListColumn::Program), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::ProcessId), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Protocol), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::LocalPort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteHostName), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemoteIp), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::RemotePort), QHeaderView::Interactive);
    header->setSectionResizeMode(int(ConnListColumn::Direction), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Action), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Reason), QHeaderView::Fixed);
    header->setSectionResizeMode(int(ConnListColumn::Time), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(ConnListColumn::Program), 300);
    header->resizeSection(int(ConnListColumn::ProcessId), 80);
    header->resizeSection(int(ConnListColumn::Protocol), 60);
    header->resizeSection(int(ConnListColumn::LocalHostName), 140);
    header->resizeSection(int(ConnListColumn::LocalIp), 100);
    header->resizeSection(int(ConnListColumn::LocalPort), 80);
    header->resizeSection(int(ConnListColumn::RemoteHostName), 170);
    header->resizeSection(int(ConnListColumn::RemoteIp), 100);
    header->resizeSection(int(ConnListColumn::RemotePort), 80);
    header->resizeSection(int(ConnListColumn::Direction), 30);
    header->resizeSection(int(ConnListColumn::Action), 30);
    header->resizeSection(int(ConnListColumn::Reason), 30);
    header->resizeSection(int(ConnListColumn::Time), 80);

    // Hidden columns
    header->setSectionHidden(int(ConnListColumn::Program), /*hide=*/true);
    header->setSectionHidden(int(ConnListColumn::LocalHostName), /*hide=*/true);
}

void ProgConnListPage::setupAppInfoRow()
{
    m_appInfoRow = new AppInfoRow();

    const auto refreshAppInfoVersion = [&] {
        m_appInfoRow->refreshAppInfoVersion(connListCurrentPath(), appInfoCache());
    };

    refreshAppInfoVersion();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshAppInfoVersion);
    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, refreshAppInfoVersion);
}

void ProgConnListPage::setupTableConnListChanged()
{
    const auto refreshTableConnListChanged = [&] {
        const int connIndex = m_connListView->currentRow();
        const bool connSelected = (connIndex >= 0);
        m_actCopyAsFilter->setEnabled(connSelected);
        m_actCopy->setEnabled(connSelected);
        m_actLookupIp->setEnabled(connSelected);
        m_appInfoRow->setVisible(connSelected);
    };

    refreshTableConnListChanged();

    connect(m_connListView, &TableView::currentIndexChanged, this, refreshTableConnListChanged);
}

int ProgConnListPage::connListCurrentIndex() const
{
    return m_connListView->currentRow();
}

const ConnRow &ProgConnListPage::connListCurrentRow() const
{
    return m_appConnListModel->connRowAt(connListCurrentIndex());
}

QString ProgConnListPage::connListCurrentPath() const
{
    const auto &connRow = connListCurrentRow();
    return connRow.isNull() ? QString() : connRow.appPath;
}
