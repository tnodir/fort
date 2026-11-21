#include "activitywindow.h"

#include <QHeaderView>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/treeview.h>
#include <form/opt/pages/optmainpage.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/activitytreemodel.h>
#include <user/iniuser.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "activitycontroller.h"

using namespace Fort;

namespace {

inline constexpr int ACTIVITY_HEADER_VERSION = 1;

}

ActivityWindow::ActivityWindow(QWidget *parent) :
    FormWindow(parent), m_ctrl(new ActivityController(this)), m_model(new ActivityTreeModel(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser());
}

QString ActivityWindow::iniGroup() const
{
    return IniUser::activityWindowGroup();
}

void ActivityWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    saveIniWindowGeometry();

    iniUser.setActivityHeader(m_treeView->header()->saveState());
    iniUser.setActivityHeaderVersion(ACTIVITY_HEADER_VERSION);

    confManager()->saveIniUser();
}

void ActivityWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    restoreIniWindowGeometry(QSize(1000, 600));

    if (iniUser.activityHeaderVersion() == ACTIVITY_HEADER_VERSION) {
        m_treeView->header()->restoreState(iniUser.activityHeader());
    }
}

void ActivityWindow::showEvent(QShowEvent *event)
{
    FormWindow::showEvent(event);

    m_model->setActive(true);
}

void ActivityWindow::hideEvent(QHideEvent *event)
{
    FormWindow::hideEvent(event);

    m_model->setActive(false);
}

void ActivityWindow::setupController()
{
    connect(ctrl(), &ActivityController::retranslateUi, this, &ActivityWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void ActivityWindow::retranslateUi()
{
    this->unsetLocale();

    m_model->refreshTexts();

    this->setWindowTitle(tr("Activity"));
}

void ActivityWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Tree
    setupTreeActivity();
    setupTreeActivityHeader();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addLayout(header);
    layout->addWidget(m_treeView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(500, 300);
}

QLayout *ActivityWindow::setupHeader()
{
    // Header buttons
    m_headerButtons = new HeaderButtons(OptMainPage::TabStatistics);

    return ControlUtil::createHLayoutByWidgets({ /*stretch*/ nullptr, m_headerButtons });
}

void ActivityWindow::setupTreeActivity()
{
    m_treeView = new TreeView();
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_treeView->setModel(m_model);

    // The rows are sorted on the header's click only
    m_treeView->header()->setSortIndicator(int(ActivityColumn::Program), Qt::AscendingOrder);
    m_treeView->setSortingEnabled(true);
}

void ActivityWindow::setupTreeActivityHeader()
{
    auto header = m_treeView->header();

    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setStretchLastSection(true);

    constexpr int columnWidths[] = { 260, 60, 160, 140, 80, 200, 200, 90, 90, 90, 90, 80, 140 };
    static_assert(std::size(columnWidths) == int(ActivityColumn::Count));

    for (int i = 0; i < int(ActivityColumn::Count); ++i) {
        header->resizeSection(i, columnWidths[i]);
    }

    // Hidden columns
    header->setSectionHidden(int(ActivityColumn::LocalAddress), /*hide=*/true);

    header->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header, &QHeaderView::customContextMenuRequested, this,
            &ActivityWindow::showTreeActivityHeaderMenu);
}

void ActivityWindow::showTreeActivityHeaderMenu(const QPoint &pos)
{
    ControlUtil::showHeaderMenu(
            m_treeView->header(), pos,
            [](int column) { return ActivityTreeModel::columnName(ActivityColumn(column)); },
            tr("Stretch last column"));
}
