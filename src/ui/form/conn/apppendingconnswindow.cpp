#include "apppendingconnswindow.h"

#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <model/apppendingconnsearchmodel.h>
#include <user/iniuser.h>

#include "connectionscontroller.h"

using namespace Fort;

namespace {

// The icon only button's mnemonic and tool tip
void setIconButtonText(QAbstractButton *bt, const QString &text)
{
    bt->setText(text);
    bt->setToolTip(QString(text).remove(QLatin1Char('&')));
}

}

AppPendingConnsWindow::AppPendingConnsWindow(const App &app, QWidget *parent) :
    AppPendingConnsWindow(app, parent, Qt::Window)
{
    initialize();

    setupFormWindow(iniUser(), IniUser::connWindowGroup());
}

AppPendingConnsWindow::AppPendingConnsWindow(const App &app, QWidget *parent, Qt::WindowFlags f) :
    AppConnsWindow(new AppPendingConnSearchModel(), app, parent, f)
{
    setupUi();
    setupController();
}

AppPendingConnSearchModel *AppPendingConnsWindow::pendingConnListModel() const
{
    return static_cast<AppPendingConnSearchModel *>(connListModel());
}

void AppPendingConnsWindow::retranslateWindowTitle()
{
    this->setWindowTitle(tr("Paused Connections"));
}

void AppPendingConnsWindow::setupController()
{
    connect(ctrl(), &ConnectionsController::retranslateUi, this,
            &AppPendingConnsWindow::retranslateUi);
}

void AppPendingConnsWindow::retranslateUi()
{
    setIconButtonText(m_btAllow, tr("&Allow once"));
    setIconButtonText(m_btBlock, tr("&Block once"));
    setIconButtonText(m_btContinue, tr("&Continue"));
    m_btContinue->setToolTip(tr("Continue by the current rules: the still asked ones are blocked"));

    // The button's text with the mnemonic
    addFilterAction()->setIconText(tr("Add &Filter"));
}

void AppPendingConnsWindow::setupUi()
{
    // Allow, Block & Continue
    setupDecideButtons();

    // Add Filter
    m_btAddFilter = ControlUtil::createIconToolButton(QString());
    m_btAddFilter->setDefaultAction(addFilterAction());

    // After the program's name
    auto layout = headerLayout();
    layout->insertWidget(2, m_btAllow);
    layout->insertWidget(3, m_btBlock);
    layout->insertWidget(4, m_btContinue);
    layout->insertWidget(5, m_btAddFilter);
    layout->insertWidget(6, ControlUtil::createVSeparator());

    // The selected row stays in view, when the new rows are added
    setAutoScroll(false);

    setupConnsChanged();
}

void AppPendingConnsWindow::setupDecideButtons()
{
    m_btAllow = ControlUtil::createIconToolButton(
            ":/icons/accept.png", [&] { decideConns(PendingConnManager::ConnAllow); });
    m_btBlock = ControlUtil::createIconToolButton(
            ":/icons/deny.png", [&] { decideConns(PendingConnManager::ConnBlock); });
    m_btContinue = ControlUtil::createIconToolButton(":/icons/arrow_refresh_small.png",
            [&] { decideConns(PendingConnManager::ConnContinue); });
}

void AppPendingConnsWindow::setupConnsChanged()
{
    updateDecideButtons();

    connect(connListView(), &TableView::currentIndexChanged, this,
            &AppPendingConnsWindow::updateDecideButtons);
    connect(connListView()->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &AppPendingConnsWindow::updateDecideButtons);
    connect(connListModel(), &QAbstractItemModel::modelReset, this,
            &AppPendingConnsWindow::updateDecideButtons);
    // The connections are no longer pending
    connect(connListModel(), &QAbstractItemModel::dataChanged, this,
            &AppPendingConnsWindow::updateDecideButtons);
}

void AppPendingConnsWindow::updateDecideButtons()
{
    const bool hasOutbound = !selectedConnIds(/*outboundOnly=*/true).isEmpty();
    const bool hasSelected = !selectedConnIds().isEmpty();

    m_btAllow->setEnabled(hasOutbound);
    m_btBlock->setEnabled(hasOutbound);
    m_btContinue->setEnabled(hasSelected);
}

void AppPendingConnsWindow::decideConns(PendingConnManager::ConnDecision decision)
{
    // The inbound connection is decided by the filters only: continued
    const bool outboundOnly = (decision != PendingConnManager::ConnContinue);

    const auto connIds = selectedConnIds(outboundOnly);

    for (const qint64 connId : connIds) {
        pendingConnManager()->decideConn(connId, decision);
    }
}

QVector<qint64> AppPendingConnsWindow::selectedConnIds(bool outboundOnly) const
{
    QVector<qint64> connIds;

    const auto model = pendingConnListModel();

    const auto rows = connListView()->selectedRows();
    for (const int row : rows) {
        // The no longer pending one is decided already
        const auto &connRow = model->connRowAt(row);
        if (connRow.isNull() || !model->isConnPending(row))
            continue;

        if (outboundOnly && connRow.inbound)
            continue;

        connIds.append(connRow.connId);
    }

    return connIds;
}
