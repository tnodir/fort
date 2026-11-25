#include "graphpage.h"

#include <QVBoxLayout>

#include <form/graph/graphoptionsarea.h>
#include <form/opt/optionscontroller.h>
#include <user/inigraph.h>

GraphPage::GraphPage(OptionsController *ctrl, QWidget *parent) : OptBasePage(ctrl, parent)
{
    setupUi();
}

void GraphPage::onResetToDefault()
{
    m_graphOptions->resetToDefault();
}

void GraphPage::onRetranslateUi()
{
    m_graphOptions->retranslateUi();
}

void GraphPage::setupUi()
{
    m_graphOptions = new GraphOptionsArea(IniGraph(iniUser()));

    connect(m_graphOptions, &GraphOptionsArea::edited, this,
            [this] { ctrl()->setIniUserEdited(); });

    auto layout = new QVBoxLayout();
    layout->addWidget(m_graphOptions, 1);

    this->setLayout(layout);
}
