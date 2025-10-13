#include "programeditcontroller.h"

#include <conf/confappmanager.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>

using namespace Fort;

ProgramEditController::ProgramEditController(QObject *parent) : BaseController(parent) { }

void ProgramEditController::initialize(const App &app, const QVector<qint64> &appIdList)
{
    m_isWildcard = app.isWildcard;
    m_app = app;
    m_appIdList = appIdList;

    emit retranslateUi();

    emit initializePage(m_app);
}

bool ProgramEditController::addOrUpdateApp(App &app, bool onlyUpdate)
{
    if (confAppManager()->addOrUpdateApp(app, onlyUpdate))
        return true;

    windowManager()->showErrorBox(
            tr("Failed to save the program!\nPlease check other program with the same path."));

    return false;
}

bool ProgramEditController::updateApp(App &app)
{
    return confAppManager()->updateApp(app);
}

bool ProgramEditController::updateAppName(qint64 appId, const QString &appName)
{
    return confAppManager()->updateAppName(appId, appName);
}

void ProgramEditController::closeWindow()
{
    emit closeDialog();
}

void ProgramEditController::switchWildcard()
{
    m_isWildcard = !m_isWildcard;

    emit isWildcardSwitched();

    emit retranslateUi();
}

void ProgramEditController::saveChanges()
{
    if (save()) {
        emit closeOnSave();
    }
}

bool ProgramEditController::save()
{
    const int appIdsCount = m_appIdList.size();
    const bool isSingleSelection = (appIdsCount <= 1);

    if (isSingleSelection && !checkValidateFields()) {
        return false;
    }

    App app;
    app.appId = m_app.appId;

    emit fillApp(app);

    // Add new app or edit non-selected app
    if (appIdsCount == 0) {
        const bool onlyUpdate = app.isValid();

        return addOrUpdateApp(app, onlyUpdate);
    }

    // Edit selected app
    if (isSingleSelection) {
        return saveApp(app);
    }

    // Edit selected apps
    return saveMulti(app);
}

bool ProgramEditController::saveApp(App &app)
{
    if (!app.isOptionsEqual(m_app)) {
        return updateApp(app);
    }

    if (!app.isNameEqual(m_app)) {
        return updateAppName(app.appId, app.appName);
    }

    return true;
}

bool ProgramEditController::saveMulti(App &app)
{
    for (qint64 appId : std::as_const(m_appIdList)) {
        const auto appRow = confAppManager()->appById(appId);

        app.appId = appId;
        app.appOriginPath = appRow.appOriginPath;
        app.appPath = appRow.appPath;
        app.appName = appRow.appName;

        if (!updateApp(app))
            return false;
    }

    return true;
}

bool ProgramEditController::checkValidateFields()
{
    bool ok = false;
    emit validateFields(ok);
    return ok;
}

void ProgramEditController::warnDangerousOption() const
{
    windowManager()->showErrorBox(
            tr("Attention: This option is very dangerous!!!\n\n"
               "Be careful when killing a system services or other important programs!\n"
               "It can cause a Windows malfunction or totally unusable."));
}

void ProgramEditController::warnRestartNeededOption() const
{
    windowManager()->showErrorBox(
            tr("Attention: This option only affects new processes!\n\n"
               "Please restart the running program to take effect of this option."));
}
