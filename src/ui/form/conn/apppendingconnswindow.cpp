#include "apppendingconnswindow.h"

#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QMenu>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <form/controls/tableview.h>
#include <fortglobal.h>
#include <model/apppendingconnsearchmodel.h>
#include <user/iniuser.h>

#include "connectionscontroller.h"

using namespace Fort;

namespace {

inline constexpr std::array allowProcessSecondValues = { 10, 30, 60 * 1, 60 * 5, 60 * 10, 60 * 30,
    60 * 60 * 1, 60 * 60 * 3, 60 * 60 * 6 };

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
    setIconButtonText(m_btAllowProcess, tr("Allow &Process"));
    m_btAllowProcess->setToolTip(tr("Allow the process till its exit or for the time by the menu"));
    retranslateAllowProcessMenu();
    setIconButtonText(m_btBlock, tr("&Block once"));
    setIconButtonText(m_btContinue, tr("&Continue"));
    m_btContinue->setToolTip(tr("Continue by the current rules: the still asked ones are blocked"));

    // The button's text with the mnemonic
    addFilterAction()->setIconText(tr("Add &Filter"));
}

void AppPendingConnsWindow::retranslateAllowProcessMenu()
{
    const QStringList list = { tr("10 seconds"), tr("30 seconds"), tr("1 minute"), tr("5 minutes"),
        tr("10 minutes"), tr("30 minutes"), tr("1 hour"), tr("3 hours"), tr("6 hours") };

    ControlUtil::setActionTexts(m_btAllowProcess->menu()->actions(), list);
}

void AppPendingConnsWindow::setupUi()
{
    // Allow, Allow Process, Block & Continue
    setupDecideButtons();

    // Add Filter
    auto btAddFilter = ControlUtil::createIconToolButton(QString());
    btAddFilter->setDefaultAction(addFilterAction());

    // After the hidden program's name
    hideAppName();

    auto layout = headerLayout();
    layout->insertWidget(2, m_btAllow);
    layout->insertWidget(3, m_btAllowProcess);
    layout->insertWidget(4, m_btBlock);
    layout->insertWidget(5, m_btContinue);
    layout->insertWidget(6, btAddFilter);
    layout->insertWidget(7, ControlUtil::createVSeparator());

    // The selected row stays in view, when the new rows are added
    setAutoScroll(false);

    setupConnsChanged();
}

void AppPendingConnsWindow::setupDecideButtons()
{
    m_btAllow = ControlUtil::createIconToolButton(
            ":/icons/accept.png", [&] { decideConns(PendingConnManager::ConnAllow); });
    setupAllowProcessButton();
    m_btBlock = ControlUtil::createIconToolButton(
            ":/icons/deny.png", [&] { decideConns(PendingConnManager::ConnBlock); });
    m_btContinue = ControlUtil::createIconToolButton(":/icons/arrow_refresh_small.png",
            [&] { decideConns(PendingConnManager::ConnContinue); });
}

void AppPendingConnsWindow::setupAllowProcessButton()
{
    auto menu = ControlUtil::createMenu(this);

    for (const int seconds : allowProcessSecondValues) {
        auto a = menu->addAction(QString());
        a->setData(seconds);
    }

    connect(menu, &QMenu::triggered, this, [&](QAction *a) { allowProcesses(a->data().toUInt()); });

    // Till the process's exit
    m_btAllowProcess = ControlUtil::createIconToolButton(
            ":/icons/tick.png", [&] { allowProcesses(/*seconds=*/0); });
    m_btAllowProcess->setPopupMode(QToolButton::MenuButtonPopup);
    m_btAllowProcess->setMenu(menu);
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
    m_btAllowProcess->setEnabled(hasSelected);
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

void AppPendingConnsWindow::allowProcesses(quint32 seconds)
{
    const auto processIds = selectedProcessIds();

    for (const quint32 processId : processIds) {
        pendingConnManager()->allowProcess(processId, seconds);
    }
}

QVector<qint64> AppPendingConnsWindow::selectedConnIds(bool outboundOnly) const
{
    QVector<qint64> connIds;

    walkSelectedPendingConns([&](const ConnRow &connRow) {
        if (outboundOnly && connRow.inbound)
            return;

        connIds.append(connRow.connId);
    });

    return connIds;
}

QVector<quint32> AppPendingConnsWindow::selectedProcessIds() const
{
    QVector<quint32> processIds;

    walkSelectedPendingConns([&](const ConnRow &connRow) {
        if (!processIds.contains(connRow.pid)) {
            processIds.append(connRow.pid);
        }
    });

    return processIds;
}

void AppPendingConnsWindow::walkSelectedPendingConns(
        const std::function<void(const ConnRow &connRow)> &func) const
{
    const auto model = pendingConnListModel();

    const auto rows = connListView()->selectedRows();
    for (const int row : rows) {
        // The no longer pending one is decided already
        const auto &connRow = model->connRowAt(row);
        if (connRow.isNull() || !model->isConnPending(row))
            continue;

        func(connRow);
    }
}
