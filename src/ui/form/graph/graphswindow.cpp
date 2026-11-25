#include "graphswindow.h"

#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/controls/headerbuttons.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <model/graphlistmodel.h>
#include <user/iniuser.h>
#include <util/iconcache.h>
#include <util/window/widgetwindow.h>

#include "grapheditdialog.h"
#include "graphscontroller.h"

using namespace Fort;

namespace {

inline constexpr int GRAPHS_HEADER_VERSION = 1;

}

GraphsWindow::GraphsWindow(QWidget *parent) :
    FormWindow(parent),
    m_ctrl(new GraphsController(this)),
    m_graphListModel(new GraphListModel(this))
{
    setupUi();
    setupController();

    setupFormWindow(iniUser());
}

QString GraphsWindow::iniGroup() const
{
    return IniUser::graphsWindowGroup();
}

void GraphsWindow::saveWindowState(bool /*wasVisible*/)
{
    auto &iniUser = Fort::iniUser();

    saveIniWindowGeometry();

    auto header = m_graphListView->horizontalHeader();
    iniUser.setGraphsHeader(header->saveState());
    iniUser.setGraphsHeaderVersion(GRAPHS_HEADER_VERSION);

    confManager()->saveIniUser();
}

void GraphsWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    restoreIniWindowGeometry(QSize(500, 400));

    if (iniUser.graphsHeaderVersion() == GRAPHS_HEADER_VERSION) {
        auto header = m_graphListView->horizontalHeader();
        header->restoreState(iniUser.graphsHeader());
    }
}

void GraphsWindow::setupController()
{
    connect(ctrl(), &GraphsController::retranslateUi, this, &GraphsWindow::retranslateUi);

    emit ctrl()->retranslateUi();
}

void GraphsWindow::retranslateUi()
{
    this->unsetLocale();

    m_btEdit->setText(tr("Edit"));
    m_actAddGraph->setText(tr("Add"));
    m_actEditGraph->setText(tr("Edit"));
    m_actRemoveGraph->setText(tr("Remove"));

    m_graphListModel->refresh();

    this->setWindowTitle(tr("Graphs"));
}

void GraphsWindow::setupUi()
{
    // Header
    auto header = setupHeader();

    // Table
    setupTableGraphs();
    setupTableGraphsHeader();

    // Actions on graphs table's current changed
    setupTableGraphsChanged();

    // Actions on graph list model's changed
    setupGraphListModelChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/6);
    layout->addWidget(createHeaderWidget(header));
    layout->addWidget(m_graphListView, 1);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Size
    this->setMinimumSize(300, 200);
}

QLayout *GraphsWindow::setupHeader()
{
    // Edit Menu
    auto editMenu = ControlUtil::createMenu(this);

    m_actAddGraph = editMenu->addAction(IconCache::icon(":/icons/add.png"), QString());
    m_actAddGraph->setObjectName("actAddGraph");
    m_actAddGraph->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_N));

    m_actEditGraph = editMenu->addAction(IconCache::icon(":/icons/pencil.png"), QString());
    m_actEditGraph->setObjectName("actEditGraph");
    m_actEditGraph->setShortcut(Qt::Key_Enter);

    m_actRemoveGraph = editMenu->addAction(IconCache::icon(":/icons/delete.png"), QString());
    m_actRemoveGraph->setObjectName("actRemoveGraph");
    m_actRemoveGraph->setShortcut(Qt::Key_Delete);

    connect(m_actAddGraph, &QAction::triggered, this, &GraphsWindow::addNewGraph);
    connect(m_actEditGraph, &QAction::triggered, this, &GraphsWindow::editSelectedGraph);
    connect(m_actRemoveGraph, &QAction::triggered, this, [&] {
        windowManager()->showConfirmBox(
                this, [&] { removeSelectedGraph(); }, tr("Are you sure to remove selected graph?"));
    });

    m_btEdit = ControlUtil::createButton(":/icons/pencil.png");
    m_btEdit->setObjectName("btEdit");
    m_btEdit->setMenu(editMenu);

    // Header buttons
    m_headerButtons = new HeaderButtons();

    auto layout =
            ControlUtil::createHLayoutByWidgets({ m_btEdit, /*stretch*/ nullptr, m_headerButtons });

    return layout;
}

void GraphsWindow::setupTableGraphs()
{
    m_graphListView = new TableView();
    m_graphListView->setObjectName("graphListView");
    m_graphListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_graphListView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_graphListView->setModel(m_graphListModel);

    m_graphListView->setMenu(m_btEdit->menu());

    connect(m_graphListView, &TableView::activated, m_actEditGraph, &QAction::trigger);
}

void GraphsWindow::setupTableGraphsHeader()
{
    auto header = m_graphListView->horizontalHeader();

    header->setSectionResizeMode(int(GraphListColumn::Name), QHeaderView::Interactive);
    header->setStretchLastSection(true);

    header->resizeSection(int(GraphListColumn::Name), 250);
}

void GraphsWindow::setupTableGraphsChanged()
{
    const auto refreshTableGraphsChanged = [&] {
        const int graphIndex = graphListCurrentIndex();
        const bool isGraphSelected = (graphIndex >= 0);

        m_actEditGraph->setEnabled(isGraphSelected);

        // The main graph is always present
        m_actRemoveGraph->setEnabled(
                isGraphSelected && m_graphListModel->graphRowAt(graphIndex).graphId != 0);
    };

    refreshTableGraphsChanged();

    connect(m_graphListView, &TableView::currentIndexChanged, this, refreshTableGraphsChanged);
}

void GraphsWindow::setupGraphListModelChanged()
{
    const auto refreshAddGraph = [&] {
        m_actAddGraph->setEnabled(iniUser().newGraphWindowId() >= 0);
    };

    refreshAddGraph();

    connect(m_graphListModel, &GraphListModel::modelReset, this, refreshAddGraph);
}

void GraphsWindow::addNewGraph()
{
    const int graphId = iniUser().newGraphWindowId();
    if (graphId < 0)
        return;

    openGraphEditForm(graphId);
}

void GraphsWindow::editSelectedGraph()
{
    const int graphIndex = graphListCurrentIndex();
    if (graphIndex < 0)
        return;

    const auto &graphRow = m_graphListModel->graphRowAt(graphIndex);

    openGraphEditForm(graphRow.graphId);
}

void GraphsWindow::openGraphEditForm(int graphId)
{
    auto dialog = new GraphEditDialog(ctrl(), graphId, this);
    ControlUtil::deleteOnClose(dialog);

    WidgetWindow::showWidget(dialog);
}

void GraphsWindow::removeSelectedGraph()
{
    const auto &graphRow = m_graphListModel->graphRowAt(graphListCurrentIndex());

    ctrl()->removeGraph(graphRow.graphId);
}

int GraphsWindow::graphListCurrentIndex() const
{
    return m_graphListView->currentRow();
}
