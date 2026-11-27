#include "graphwindow.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>

#include <conf/confmanager.h>
#include <fortglobal.h>
#include <user/iniuser.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

using namespace Fort;

namespace {

inline constexpr int stickyDistance = 30;

inline constexpr QChar inArrow = QChar(0x2193); // Down
inline constexpr QChar outArrow = QChar(0x2191); // Up

// The window's shift to stick its edge to the screen's near one on an axis, else 0
int stickyEdgeDiff(int screenStart, int screenEnd, int winStart, int winEnd)
{
    const int startDiff = screenStart - winStart;
    if (qAbs(startDiff) < stickyDistance)
        return startDiff;

    const int endDiff = screenEnd - winEnd;

    return (qAbs(endDiff) < stickyDistance) ? endDiff : 0;
}

}

GraphWindow::GraphWindow(int graphId, QWidget *parent) : GraphBaseWindow(graphId, parent)
{
    setupUi();
    setupWindowOptions();

    setupFormWindow(iniUser());

    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphWindow::updateWindowGeometry);
}

QList<GraphSide> GraphWindow::popupSides() const
{
    return { GraphSideTop, GraphSideBottom, GraphSideRight, GraphSideLeft };
}

bool GraphWindow::popupOnTop() const
{
    return iniGraph().alwaysOnTop();
}

bool GraphWindow::isWindowShown() const
{
    // Hidden on hover till the mouse leaves it
    return isVisible() || isCursorLeaveWatched();
}

void GraphWindow::saveWindowState(bool wasVisible)
{
    setIniGeometry();

    GraphBaseWindow::saveWindowState(wasVisible);
}

void GraphWindow::restoreWindowState()
{
    const IniGraph &ini = iniGraph();

    m_iniGeometry = ini.geometry();
    m_iniMaximized = ini.maximized();

    stateWatcher()->restore(this, QSize(400, 300), m_iniGeometry, m_iniMaximized);
}

void GraphWindow::onWindowClosed()
{
    stopCursorLeaveWatch();
}

void GraphWindow::saveWindowGeometry()
{
    setIniGeometry();

    confManager()->saveIniUser();
}

void GraphWindow::updateWindowGeometry()
{
    const IniGraph &ini = iniGraph();

    if (ini.geometry() == m_iniGeometry && ini.maximized() == m_iniMaximized)
        return;

    restoreWindowState();
}

void GraphWindow::setIniGeometry()
{
    IniGraph &ini = iniGraph();

    m_iniGeometry = stateWatcher()->geometry();
    m_iniMaximized = stateWatcher()->maximized();

    ini.setGeometry(m_iniGeometry);
    ini.setMaximized(m_iniMaximized);
}

void GraphWindow::setupUi()
{
    connect(view(), &GraphView::speedUpdated, this, &GraphWindow::updateWindowTitle);

    // Interactions
    const auto plot = this->plot();

    connect(plot, &GraphPlot::mouseDoubleClick, this, &GraphWindow::onMouseDoubleClick);

    connect(plot, &GraphPlot::mouseDragBegin, this, &GraphWindow::onMouseDragBegin);
    connect(plot, &GraphPlot::mouseDragMove, this, &GraphWindow::onMouseDragMove);
    connect(plot, &GraphPlot::mouseDragEnd, this, &GraphWindow::onMouseDragEnd);

    setMinimumSize(QSize(30, 10));
}

void GraphWindow::setupWindowOptions()
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    updateWindowOptions();

    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphWindow::updateWindowOptions);
}

void GraphWindow::updateWindowOptions(bool onlyFlags)
{
    if (onlyFlags)
        return;

    const IniGraph &ini = iniGraph();

    updateWindowFlags(ini);

    setWindowOpacityPercent(ini.opacity());
}

void GraphWindow::updateWindowFlags(const IniGraph &ini)
{
    const bool visible = isVisible();

    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint
            | (ini.alwaysOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget)
            | (ini.frameless() ? Qt::FramelessWindowHint : Qt::Widget)
            | (ini.clickThrough() ? Qt::WindowTransparentForInput : Qt::Widget));

    if (visible) {
        show(); // setWindowFlags() hides the window
    }
}

void GraphWindow::onMouseDoubleClick(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    if (isMaximized() || isFullScreen()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void GraphWindow::onMouseDragBegin(QMouseEvent *event)
{
    m_mousePressPoint = GuiUtil::globalPos(event);
    m_posOnMousePress = this->pos();
    m_sizeOnMousePress = this->size();
    m_mouseDragResize = (event->buttons() & Qt::RightButton) != 0;

    QGuiApplication::setOverrideCursor(m_mouseDragResize ? Qt::SizeFDiagCursor : Qt::SizeAllCursor);
}

void GraphWindow::onMouseDragMove(QMouseEvent *event)
{
    if (isMaximized() || isFullScreen())
        return;

    const QPoint offset = GuiUtil::globalPos(event) - m_mousePressPoint;

    if (m_mouseDragResize) {
        resize(qMax(m_sizeOnMousePress.width() + offset.x(), minimumSize().width()),
                qMax(m_sizeOnMousePress.height() + offset.y(), minimumSize().height()));
    } else {
        move(m_posOnMousePress + offset);
    }
}

void GraphWindow::onMouseDragEnd(QMouseEvent *event)
{
    QGuiApplication::restoreOverrideCursor();

    if (event->modifiers() == Qt::NoModifier) {
        checkWindowEdges();
    }
}

void GraphWindow::cancelMousePressAndDragging()
{
    const auto plot = this->plot();

    if (!plot->mousePressed())
        return;

    if (plot->mouseDragging()) {
        QGuiApplication::restoreOverrideCursor();

        // Restore original position & size
        move(m_posOnMousePress);
        resize(m_sizeOnMousePress);
    }

    plot->cancelMousePressAndDragging();
}

void GraphWindow::enterEvent(QEnterEvent *event)
{
    if (iniGraph().hideOnHover()) {
        hide();
        startCursorLeaveWatch();
        return;
    }

    GraphBaseWindow::enterEvent(event);

    setWindowOpacityPercent(iniGraph().hoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    GraphBaseWindow::leaveEvent(event);

    setWindowOpacityPercent(iniGraph().opacity());
}

void GraphWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_Escape: // Esc
        if (event->modifiers() == Qt::NoModifier) {
            cancelMousePressAndDragging();
        }
        break;
    }
}

void GraphWindow::onCursorLeft()
{
    if (isHidden()) {
        show(); // hidden on hover
        return;
    }

    GraphBaseWindow::onCursorLeft();
}

void GraphWindow::updateWindowTitle(const QString &inText, const QString &outText)
{
    if ((windowFlags() & Qt::FramelessWindowHint) != 0)
        return;

    setWindowTitle(inArrow + inText + "  " + outArrow + outText);
}

void GraphWindow::setWindowOpacityPercent(int percent)
{
    setWindowOpacity(qreal(qBound(1, percent, 100)) / 100.0f);
}

void GraphWindow::checkWindowEdges()
{
    const auto screen = this->screen();
    if (!screen)
        return;

    const QRect screenRect = screen->geometry();
    const QRect winRect = this->frameGeometry();
    const QPoint diff(
            stickyEdgeDiff(screenRect.x(), screenRect.right(), winRect.x(), winRect.right()),
            stickyEdgeDiff(screenRect.y(), screenRect.bottom(), winRect.y(), winRect.bottom()));

    if (!diff.isNull()) {
        this->move(winRect.topLeft() + diff);
    }
}
