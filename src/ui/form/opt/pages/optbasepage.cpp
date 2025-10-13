#include "optbasepage.h"

#include <form/opt/optionscontroller.h>

OptBasePage::OptBasePage(OptionsController *ctrl, QWidget *parent) : QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

FirewallConf *OptBasePage::conf() const
{
    return ctrl()->confToEdit();
}

IniOptions &OptBasePage::ini() const
{
    auto ini = ctrl()->iniOptToEdit();
    Q_ASSERT(ini);

    return *ini;
}

IniUser &OptBasePage::iniUser() const
{
    auto iniUser = ctrl()->iniUserToEdit();
    Q_ASSERT(iniUser);

    return *iniUser;
}

void OptBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &OptionsController::aboutToSave, this, &OptBasePage::onAboutToSave);
    connect(ctrl(), &OptionsController::editResetted, this, &OptBasePage::onEditResetted);
    connect(ctrl(), &OptionsController::resetToDefault, this, &OptBasePage::onResetToDefault);

    connect(ctrl(), &OptionsController::afterSaveWindowState, this,
            &OptBasePage::onSaveWindowState);
    connect(ctrl(), &OptionsController::afterRestoreWindowState, this,
            &OptBasePage::onRestoreWindowState);

    connect(ctrl(), &OptionsController::retranslateUi, this, &OptBasePage::onRetranslateUi);
}
