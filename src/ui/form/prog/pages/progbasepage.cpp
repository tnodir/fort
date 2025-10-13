#include "progbasepage.h"

#include <form/prog/programeditcontroller.h>
#include <fortglobal.h>

using namespace Fort;

ProgBasePage::ProgBasePage(ProgramEditController *ctrl, QWidget *parent) :
    QFrame(parent), m_ctrl(ctrl)
{
    setupController();
}

const App &ProgBasePage::app() const
{
    return ctrl()->app();
}

bool ProgBasePage::isWildcard() const
{
    return ctrl()->isWildcard();
}

bool ProgBasePage::isNew() const
{
    return ctrl()->isNew();
}

bool ProgBasePage::isSingleSelection() const
{
    return ctrl()->isSingleSelection();
}

void ProgBasePage::setupController()
{
    Q_ASSERT(ctrl());

    connect(ctrl(), &ProgramEditController::initializePage, this, &ProgBasePage::onPageInitialize);
    connect(ctrl(), &ProgramEditController::retranslateUi, this, &ProgBasePage::onRetranslateUi);
}
