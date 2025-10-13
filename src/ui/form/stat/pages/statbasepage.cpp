#include "statbasepage.h"

#include <QGuiApplication>

#include <form/stat/statisticscontroller.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/iniuser.h>

using namespace Fort;

StatBasePage::StatBasePage(StatisticsController *ctrl, QWidget *parent) :
    QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

void StatBasePage::saveTabIndex(const QString &iniKey, int tabIndex)
{
    if (QGuiApplication::keyboardModifiers() != Qt::ControlModifier)
        return;

    windowManager()->showConfirmBox([=, this] { iniUser().setValue(iniKey, tabIndex); },
            tr("Make this tab active when window opens?"));
}

void StatBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &StatisticsController::afterSaveWindowState, this,
            &StatBasePage::onSaveWindowState);
    connect(ctrl(), &StatisticsController::afterRestoreWindowState, this,
            &StatBasePage::onRestoreWindowState);

    connect(ctrl(), &StatisticsController::retranslateUi, this, &StatBasePage::onRetranslateUi);
}
