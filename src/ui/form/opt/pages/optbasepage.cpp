#include "optbasepage.h"

#include <QLabel>

#include <conf/inioptions.h>
#include <form/controls/controlutil.h>
#include <form/opt/optionscontroller.h>
#include <fortglobal.h>
#include <fortsettings.h>
#include <user/iniuser.h>

namespace {

// The setter of the changed option of the ini got by the function: marks the ini edited
template<typename Ini, typename T, typename IniFunc, typename EditedFunc>
std::function<void(T)> iniSetter(const IniFunc &iniFunc, T (Ini::*iniValue)() const,
        void (Ini::*setIniValue)(T), const EditedFunc &setEdited)
{
    return [=](T v) {
        Ini &ini = iniFunc();
        if ((ini.*iniValue)() == v)
            return;

        (ini.*setIniValue)(v);
        setEdited();
    };
}

template<typename T>
std::function<void(T)> iniOptionSetter(
        OptBasePage *page, T (IniOptions::*iniValue)() const, void (IniOptions::*setIniValue)(T))
{
    return iniSetter([page]() -> IniOptions & { return page->ini(); }, iniValue, setIniValue,
            [page] { page->ctrl()->setIniEdited(); });
}

template<typename T>
std::function<void(T)> iniUserSetter(OptBasePage *page, T (IniUser::*iniValue)() const,
        void (IniUser::*setIniValue)(T), bool flagsChanged)
{
    return iniSetter([page]() -> IniUser & { return page->iniUser(); }, iniValue, setIniValue,
            [page, flagsChanged] { page->ctrl()->setIniUserEdited(flagsChanged); });
}

}

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

QWidget *OptBasePage::createWarning(QLabel *label)
{
    auto iconWarning = ControlUtil::createIconLabel(":/icons/error.png", QSize(16, 16));

    label->setWordWrap(true);

    auto layout = ControlUtil::createHLayoutByWidgets({ iconWarning, label }, /*margin=*/0);

    return ControlUtil::wrapToWidget(layout);
}

QWidget *OptBasePage::createNoServiceWarning(QLabel *label)
{
    auto w = createWarning(label);

    // Shown with its parent: a parentless widget shown is a window
    if (Fort::settings()->hasService()) {
        w->hide();
    }

    return w;
}

QCheckBox *OptBasePage::createIniCheckBox(
        bool (IniOptions::*iniFlag)() const, void (IniOptions::*setIniFlag)(bool))
{
    return ControlUtil::createCheckBox(
            (ini().*iniFlag)(), iniOptionSetter(this, iniFlag, setIniFlag));
}

std::function<void(int)> OptBasePage::iniValueSetter(
        int (IniOptions::*iniValue)() const, void (IniOptions::*setIniValue)(int))
{
    return iniOptionSetter(this, iniValue, setIniValue);
}

QCheckBox *OptBasePage::createIniUserCheckBox(
        bool (IniUser::*iniFlag)() const, void (IniUser::*setIniFlag)(bool), bool flagsChanged)
{
    return ControlUtil::createCheckBox(
            (iniUser().*iniFlag)(), iniUserSetter(this, iniFlag, setIniFlag, flagsChanged));
}

std::function<void(int)> OptBasePage::iniUserValueSetter(
        int (IniUser::*iniValue)() const, void (IniUser::*setIniValue)(int))
{
    return iniUserSetter(this, iniValue, setIniValue, /*flagsChanged=*/false);
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
