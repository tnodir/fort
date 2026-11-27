#include "graphbasewindow.h"

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/graph/graphpopup.h>
#include <fortglobal.h>

using namespace Fort;

GraphBaseWindow::GraphBaseWindow(int graphId, QWidget *parent) : FormWindow(parent)
{
    setupUi(graphId);
}

GraphBaseWindow::~GraphBaseWindow()
{
    closePopup();
}

QString GraphBaseWindow::iniGroup() const
{
    return iniGraph().group();
}

bool GraphBaseWindow::deleteOnClose() const
{
    return !iniGraph().hideOnClose();
}

bool GraphBaseWindow::isHostChanged() const
{
    return iniGraph().showInTaskbar() != inTaskbar();
}

void GraphBaseWindow::saveWindowState(bool wasVisible)
{
    iniGraph().setVisible(wasVisible);

    confManager()->saveIniUser();
}

void GraphBaseWindow::enterEvent(QEnterEvent *event)
{
    FormWindow::enterEvent(event);

    showPopup();
}

void GraphBaseWindow::leaveEvent(QEvent *event)
{
    FormWindow::leaveEvent(event);

    closePopup();
}

void GraphBaseWindow::hideEvent(QHideEvent *event)
{
    FormWindow::hideEvent(event);

    closePopup();
}

void GraphBaseWindow::setupUi(int graphId)
{
    m_view = new GraphView(graphId);

    auto layout = ControlUtil::createVLayoutByWidgets({ m_view }, /*margin=*/0);
    setLayout(layout);
}

void GraphBaseWindow::showPopup()
{
    if (!m_popup) {
        m_popup = new GraphPopup(this);
    }

    m_popup->showPopup();
}

void GraphBaseWindow::closePopup()
{
    delete m_popup;
    m_popup = nullptr;
}
