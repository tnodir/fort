#include "optbasepage.h"

#include <QLabel>

#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <fortsettings.h>

OptBasePage::OptBasePage(OptionsController *ctrl, QWidget *parent) : QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

FirewallConf &OptBasePage::conf() const
{
    return ctrl()->confToEdit();
}

IniOptions &OptBasePage::ini() const
{
    return ctrl()->iniOptToEdit();
}

IniUser &OptBasePage::iniUser() const
{
    return ctrl()->iniUserToEdit();
}

QWidget *OptBasePage::createNoServiceWarning(QLabel *label)
{
    auto iconWarning = ControlUtil::createIconLabel(":/icons/error.png", QSize(16, 16));

    label->setWordWrap(true);

    auto layout = ControlUtil::createHLayoutByWidgets({ iconWarning, label }, /*margin=*/0);

    auto w = ControlUtil::wrapToWidget(layout);
    w->setVisible(!Fort::settings()->hasService());

    return w;
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
