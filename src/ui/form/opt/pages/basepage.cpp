#include "basepage.h"

#include "../optionscontroller.h"

BasePage::BasePage(OptionsController *ctrl,
                   QWidget *parent) :
    QFrame(parent),
    m_ctrl(ctrl)
{
    setupController();
}

FortManager *BasePage::fortManager() const
{
    return ctrl()->fortManager();
}

FortSettings *BasePage::settings() const
{
    return ctrl()->settings();
}

FirewallConf *BasePage::conf() const
{
    return ctrl()->conf();
}

DriverManager *BasePage::driverManager() const
{
    return ctrl()->driverManager();
}

TranslationManager *BasePage::translationManager() const
{
    return ctrl()->translationManager();
}

TaskManager *BasePage::taskManager() const
{
    return ctrl()->taskManager();
}

void BasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &OptionsController::editResetted, this, &BasePage::onEditResetted);
    connect(ctrl(), &OptionsController::aboutToSave, this, &BasePage::onAboutToSave);
    connect(ctrl(), &OptionsController::saved, this, &BasePage::onSaved);

    connect(ctrl(), &OptionsController::retranslateUi, this, &BasePage::onRetranslateUi);
}
