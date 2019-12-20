#include "basepage.h"

#include <QCheckBox>
#include <QComboBox>

#include "../optionscontroller.h"

BasePage::BasePage(OptionsController *ctrl,
                   QWidget *parent) :
    QFrame(parent),
    m_ctrl(ctrl)
{
    setupController();
}

FortSettings *BasePage::settings()
{
    return ctrl()->settings();
}

FirewallConf *BasePage::conf()
{
    return ctrl()->conf();
}

void BasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &OptionsController::editResetted, this, &BasePage::onEditResetted);
    connect(ctrl(), &OptionsController::aboutToSave, this, &BasePage::onAboutToSave);
    connect(ctrl(), &OptionsController::saved, this, &BasePage::onSaved);

    connect(ctrl(), &OptionsController::retranslateUi, this, &BasePage::onRetranslateUi);
}

QCheckBox *BasePage::createCheckBox(bool checked,
                                    const std::function<void (bool checked)> &onToggled)
{
    auto cb = new QCheckBox();
    cb->setChecked(checked);

    connect(cb, &QCheckBox::toggled, onToggled);

    return cb;
}

QComboBox *BasePage::createComboBox(const QStringList &texts,
                                    const std::function<void (int index)> &onActivated)
{
    auto cb = new QComboBox();
    cb->addItems(texts);

    connect(cb, QOverload<int>::of(&QComboBox::activated), onActivated);

    return cb;
}
