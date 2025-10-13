#include "homebasepage.h"

#include <form/home/homecontroller.h>

HomeBasePage::HomeBasePage(HomeController *ctrl, QWidget *parent) : QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

void HomeBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &HomeController::afterSaveWindowState, this, &HomeBasePage::onSaveWindowState);
    connect(ctrl(), &HomeController::afterRestoreWindowState, this,
            &HomeBasePage::onRestoreWindowState);

    connect(ctrl(), &HomeController::retranslateUi, this, &HomeBasePage::onRetranslateUi);
    connect(ctrl(), &HomeController::passwordLockedChanged, this,
            &HomeBasePage::onPasswordLockedChanged);

    onPasswordLockedChanged();
}

void HomeBasePage::onPasswordLockedChanged()
{
    setEnabled(!ctrl()->passwordLocked());
}
