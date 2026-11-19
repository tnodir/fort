#include "headerbuttons.h"

#include <QBoxLayout>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>

#include "controlutil.h"
#include "windowmenubutton.h"

HeaderButtons::HeaderButtons(int optionsTabIndex, QWidget *parent) : QWidget(parent)
{
    setupUi(optionsTabIndex);
}

void HeaderButtons::setSeparatorVisible(bool visible)
{
    m_separator->setVisible(visible);
}

void HeaderButtons::setupUi(int optionsTabIndex)
{
    // Separator: shown after the window's own controls
    m_separator = ControlUtil::createVSeparator();
    m_separator->setVisible(false);

    // Options button
    m_btOptions = ControlUtil::createOptionsButton(optionsTabIndex);

    // Statistics button
    m_btStatistics = ControlUtil::createStatisticsButton();

    // Menu button
    m_btMenu = ControlUtil::createMenuButton();

    // Window Menu button: shown by a dockable window
    m_btWindowMenu = new WindowMenuButton();
    m_btWindowMenu->setVisible(false);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_separator, m_btOptions, m_btStatistics, m_btMenu, m_btWindowMenu }, /*margin=*/0);

    this->setLayout(layout);
}
