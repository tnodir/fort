#include "graphbasewindow.h"

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <fortglobal.h>

using namespace Fort;

GraphBaseWindow::GraphBaseWindow(int graphId, QWidget *parent) : FormWindow(parent)
{
    setupUi(graphId);
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

void GraphBaseWindow::setupUi(int graphId)
{
    m_view = new GraphView(graphId);

    auto layout = ControlUtil::createVLayoutByWidgets({ m_view }, /*margin=*/0);
    setLayout(layout);
}
