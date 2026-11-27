#include "graphbasewindow.h"

#include <QCursor>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/graph/graphpopup.h>
#include <fortglobal.h>

using namespace Fort;

namespace {

inline constexpr int cursorLeaveCheckMsec = 150;

}

GraphBaseWindow::GraphBaseWindow(int graphId, QWidget *parent) : FormWindow(parent)
{
    setupUi(graphId);
    setupCursorLeaveTimer();
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

QRect GraphBaseWindow::globalFrameGeometry() const
{
    const QRect frame = frameGeometry();
    const QPoint frameOffset = frame.topLeft() - geometry().topLeft();

    return QRect(mapToGlobal(frameOffset), frame.size());
}

void GraphBaseWindow::enterEvent(QEnterEvent *event)
{
    FormWindow::enterEvent(event);

    showPopup();
}

void GraphBaseWindow::leaveEvent(QEvent *event)
{
    FormWindow::leaveEvent(event);

    // A layered window's transparent pixel doesn't take the mouse: the graph scrolled under a still
    // cursor leaves it. No other leave follows, so the cursor is polled
    if (m_popup && isCursorInFrame()) {
        startCursorLeaveWatch();
        return;
    }

    closePopup();
}

void GraphBaseWindow::hideEvent(QHideEvent *event)
{
    FormWindow::hideEvent(event);

    closePopup();
}

void GraphBaseWindow::startCursorLeaveWatch()
{
    m_cursorLeaveTimer.start();
}

void GraphBaseWindow::stopCursorLeaveWatch()
{
    m_cursorLeaveTimer.stop();
}

void GraphBaseWindow::onCursorLeft()
{
    closePopup();
}

void GraphBaseWindow::setupUi(int graphId)
{
    m_view = new GraphView(graphId);

    auto layout = ControlUtil::createVLayoutByWidgets({ m_view }, /*margin=*/0);
    setLayout(layout);
}

void GraphBaseWindow::setupCursorLeaveTimer()
{
    connect(&m_cursorLeaveTimer, &QTimer::timeout, this, &GraphBaseWindow::checkCursorLeave);

    m_cursorLeaveTimer.setInterval(cursorLeaveCheckMsec);
}

void GraphBaseWindow::showPopup()
{
    stopCursorLeaveWatch();

    if (!m_popup) {
        m_popup = new GraphPopup(this);
    }

    m_popup->showPopup();
}

void GraphBaseWindow::closePopup()
{
    if (!m_popup)
        return; // the watch of a window hidden on hover goes on

    stopCursorLeaveWatch();

    delete m_popup;
    m_popup = nullptr;
}

bool GraphBaseWindow::isCursorInFrame() const
{
    return globalFrameGeometry().contains(QCursor::pos());
}

void GraphBaseWindow::checkCursorLeave()
{
    if (isCursorInFrame())
        return;

    stopCursorLeaveWatch();

    onCursorLeft();
}
