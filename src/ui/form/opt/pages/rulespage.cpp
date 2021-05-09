#include "rulespage.h"

RulesPage::RulesPage(OptionsController *ctrl, QWidget *parent) : BasePage(ctrl, parent)
{
    setupUi();
}

void RulesPage::onSaveWindowState(IniOptions * /*ini*/) { }

void RulesPage::onRestoreWindowState(IniOptions * /*ini*/) { }

void RulesPage::onRetranslateUi() { }

void RulesPage::setupUi() { }
