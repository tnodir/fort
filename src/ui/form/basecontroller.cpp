#include "basecontroller.h"

#include <fortglobal.h>
#include <manager/translationmanager.h>

BaseController::BaseController(QObject *parent) : QObject(parent)
{
    connect(Fort::translationManager(), &TranslationManager::languageChanged, this,
            &BaseController::retranslateUi);
}
