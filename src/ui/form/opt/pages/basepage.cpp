#include "basepage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

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

QPushButton *BasePage::createButton(const std::function<void ()> &onClicked)
{
    auto cb = new QPushButton();

    connect(cb, &QPushButton::clicked, onClicked);

    return cb;
}

QPushButton *BasePage::createLinkButton(const QString &iconPath,
                                        const QString &linkPath,
                                        const QString &toolTip)
{
    auto bt = new QPushButton(QIcon(iconPath), QString());
    bt->setWindowFilePath(linkPath);
    bt->setToolTip(!toolTip.isEmpty() ? toolTip : linkPath);
    return bt;
}
