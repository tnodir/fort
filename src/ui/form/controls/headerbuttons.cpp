#include "headerbuttons.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QToolButton>

#include "controlutil.h"

HeaderButtons::HeaderButtons(int optionsTabIndex, QWidget *parent) : QWidget(parent)
{
    setupUi(optionsTabIndex);
}

void HeaderButtons::setupUi(int optionsTabIndex)
{
    // Options button
    m_btOptions = ControlUtil::createOptionsButton(optionsTabIndex);

    // Statistics button
    m_btStatistics = ControlUtil::createStatisticsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_btOptions, m_btStatistics, m_btMenu }, /*margin=*/0);

    this->setLayout(layout);
}
