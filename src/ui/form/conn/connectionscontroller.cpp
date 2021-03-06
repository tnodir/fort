#include "connectionscontroller.h"

#include "../../conf/confmanager.h"
#include "../../fortmanager.h"
#include "../../translationmanager.h"

ConnectionsController::ConnectionsController(FortManager *fortManager, QObject *parent) :
    QObject(parent), m_fortManager(fortManager)
{
    connect(translationManager(), &TranslationManager::languageChanged, this,
            &ConnectionsController::retranslateUi);
}

FortSettings *ConnectionsController::settings() const
{
    return fortManager()->settings();
}

ConfManager *ConnectionsController::confManager() const
{
    return fortManager()->confManager();
}

FirewallConf *ConnectionsController::conf() const
{
    return confManager()->conf();
}

ConnListModel *ConnectionsController::connListModel() const
{
    return fortManager()->connListModel();
}

TranslationManager *ConnectionsController::translationManager() const
{
    return TranslationManager::instance();
}
