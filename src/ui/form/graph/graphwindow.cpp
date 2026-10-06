#include "graphwindow.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QStyleHints>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <form/tray/trayicon.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <stat/statmanager.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/guiutil.h>
#include <util/window/widgetwindowstatewatcher.h>

using namespace Fort;

namespace {

inline constexpr int stickyDistance = 30;

inline void checkWindowHorizontalEdges(const QRect &screenRect, const QRect &winRect, QPoint &diff)
{
    const int leftDiff = screenRect.x() - winRect.x();
    if (qAbs(leftDiff) < stickyDistance) {
        diff.setX(leftDiff);
    } else {
        const int rightDiff = screenRect.width() - winRect.right();
        if (qAbs(rightDiff) < stickyDistance) {
            diff.setX(rightDiff);
        }
    }
}

inline void checkWindowVerticalEdges(const QRect &screenRect, const QRect &winRect, QPoint &diff)
{
    const int topDiff = screenRect.y() - winRect.y();
    if (qAbs(topDiff) < stickyDistance) {
        diff.setY(topDiff);
    } else {
        const int bottomDiff = screenRect.height() - winRect.bottom();
        if (qAbs(bottomDiff) < stickyDistance) {
            diff.setY(bottomDiff);
        }
    }
}

}

GraphWindow::GraphWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();
    setupFlagsAndColors();
    setupTimer();

    setupFormWindow(iniUser(), IniUser::graphWindowGroup());

    connect(this, &GraphWindow::mouseRightClick, this, [&](QMouseEvent *event) {
        windowManager()->trayIcon()->showTrayMenu(GuiUtil::globalPos(event));
    });

    connect(statManager(), &StatManager::trafficAdded, this, &GraphWindow::addTraffic);
}

bool GraphWindow::deleteOnClose() const
{
    return !iniUser().graphWindowHideOnClose();
}

void GraphWindow::saveWindowState(bool wasVisible)
{
    auto &iniUser = Fort::iniUser();

    iniUser.setGraphWindowGeometry(stateWatcher()->geometry());
    iniUser.setGraphWindowMaximized(stateWatcher()->maximized());

    iniUser.setGraphWindowVisible(wasVisible);

    confManager()->saveIniUser();
}

void GraphWindow::restoreWindowState()
{
    const auto &iniUser = Fort::iniUser();

    stateWatcher()->restore(
            this, QSize(400, 300), iniUser.graphWindowGeometry(), iniUser.graphWindowMaximized());
}

void GraphWindow::setupUi()
{
    m_plot = new GraphPlot();

    // Interactions
    connect(m_plot, &GraphPlot::resized, this, &GraphWindow::updateGraph);

    connect(m_plot, &GraphPlot::mouseDoubleClick, this, &GraphWindow::onMouseDoubleClick);
    connect(m_plot, &GraphPlot::mouseRightClick, this, &GraphWindow::mouseRightClick);

    connect(m_plot, &GraphPlot::mouseDragBegin, this, &GraphWindow::onMouseDragBegin);
    connect(m_plot, &GraphPlot::mouseDragMove, this, &GraphWindow::onMouseDragMove);
    connect(m_plot, &GraphPlot::mouseDragEnd, this, &GraphWindow::onMouseDragEnd);

    // Widget Layout
    auto layout = ControlUtil::createVLayoutByWidgets({ m_plot }, /*margin=*/0);
    setLayout(layout);

    setMinimumSize(QSize(30, 10));
}

void GraphWindow::setupFlagsAndColors()
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    forceUpdateFlagsAndColors();

    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphWindow::updateFlagsAndColors);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QApplication::styleHints(), &QStyleHints::colorSchemeChanged, this,
            &GraphWindow::forceUpdateFlagsAndColors);
#endif
}

void GraphWindow::forceUpdateFlagsAndColors()
{
    updateFlagsAndColors();
}

void GraphWindow::updateFlagsAndColors(bool onlyFlags)
{
    if (onlyFlags)
        return;

    const auto &ini = Fort::iniUser();

    updateWindowFlags(ini);
    updateColors(ini);
    updateFonts(ini);
    updateFormat(ini);

    updateGraph();
}

void GraphWindow::updateWindowFlags(const IniUser &ini)
{
    const bool visible = isVisible();

    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint
            | (ini.graphWindowAlwaysOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget)
            | (ini.graphWindowFrameless() ? Qt::FramelessWindowHint : Qt::Widget)
            | (ini.graphWindowClickThrough() ? Qt::WindowTransparentForInput : Qt::Widget));

    if (visible) {
        show(); // setWindowFlags() hides the window
    }
}

void GraphWindow::updateColors(const IniUser &ini)
{
    setWindowOpacityPercent(ini.graphWindowOpacity());

    m_plot->setColors(getColors(ini));
}

void GraphWindow::updateFonts(const IniUser &ini)
{
    m_plot->setTickLabelSize(ini.graphWindowTickLabelSize());
    m_plot->setSpeedVisible(ini.graphWindowShowSpeed());
}

void GraphWindow::updateFormat(const IniUser &ini)
{
    m_plot->setUnitFormat(FormatUtil::graphUnitFormat(ini.graphWindowTrafUnit()));
}

void GraphWindow::setupTimer()
{
    connect(&m_hoverTimer, &QTimer::timeout, this, &GraphWindow::checkHoverLeave);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphWindow::updateGraph);

    m_hoverTimer.setInterval(300);
    m_updateTimer.setInterval(1000); // 1 second

    m_updateTimer.start();
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
    if (!m_plot->mousePressed())
        return;

    if (m_plot->mouseDragging()) {
        QGuiApplication::restoreOverrideCursor();

        // Restore original position & size
        move(m_posOnMousePress);
        resize(m_sizeOnMousePress);
    }

    m_plot->cancelMousePressAndDragging();
}

void GraphWindow::showEvent(QShowEvent *event)
{
    FormWindow::showEvent(event);

    updateGraph(); // the hidden window isn't updated
}

void GraphWindow::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);

    if (iniUser().graphWindowHideOnHover()) {
        hide();
        m_hoverTimer.start();
        return;
    }

    setWindowOpacityPercent(iniUser().graphWindowHoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    setWindowOpacityPercent(iniUser().graphWindowOpacity());
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

void GraphWindow::checkHoverLeave()
{
    const QPoint mousePos = QCursor::pos();

    if (!geometry().contains(mousePos)) {
        m_hoverTimer.stop();
        show();
    }
}

void GraphWindow::addTraffic(qint64 unixTime, quint64 inBytes, quint64 outBytes)
{
    const qint64 rangeLower = unixTime - iniUser().graphWindowMaxSeconds();

    m_plot->addPoint({ unixTime, inBytes * 8, outBytes * 8 }, rangeLower);
}

void GraphWindow::updateGraph()
{
    const qint64 unixTime = DateUtil::getUnixTime();

    // Move the graph to the current time
    addTraffic(unixTime, 0, 0);

    if (!isVisible())
        return;

    updateSpeed(unixTime - 1); // the last complete second

    m_plot->setFixedValueMax(iniUser().graphWindowFixedSpeed() * 1024LL);

    m_plot->replot();
}

void GraphWindow::updateSpeed(qint64 unixTime)
{
    const bool showTextSpeed = m_plot->speedVisible();
    const bool showWindowSpeed = (windowFlags() & Qt::FramelessWindowHint) == 0;

    if (!(showTextSpeed || showWindowSpeed))
        return;

    const auto text = getSpeedText(unixTime);

    if (showTextSpeed) {
        m_plot->setSpeedText(text);
    }

    if (showWindowSpeed) {
        setWindowTitle(text);
    }
}

QString GraphWindow::getSpeedText(qint64 unixTime) const
{
    const GraphPoint point = m_plot->pointAt(unixTime);
    const auto unitFormat = m_plot->unitFormat();

    return QChar(0x2193) // ↓
            + FormatUtil::formatSpeed(qint64(point.inBits), unitFormat) + "  " + QChar(0x2191) // ↑
            + FormatUtil::formatSpeed(qint64(point.outBits), unitFormat);
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
    QPoint diff(0, 0);

    checkWindowHorizontalEdges(screenRect, winRect, diff);
    checkWindowVerticalEdges(screenRect, winRect, diff);

    if (diff.x() != 0 || diff.y() != 0) {
        this->move(winRect.x() + diff.x(), winRect.y() + diff.y());
    }
}

GraphPlot::ColorArray GraphWindow::getColors(const IniUser &ini)
{
    GraphPlot::ColorArray colors;

    const bool isLightTheme =
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark;
#else
            true;
#endif

    if (isLightTheme) {
        colors << ini.graphWindowColor() << ini.graphWindowColorIn() << ini.graphWindowColorOut()
               << ini.graphWindowAxisColor() << ini.graphWindowTickLabelColor()
               << ini.graphWindowLabelColor() << ini.graphWindowGridColor();
    } else {
        colors << ini.graphWindowDarkColor() << ini.graphWindowDarkColorIn()
               << ini.graphWindowDarkColorOut() << ini.graphWindowDarkAxisColor()
               << ini.graphWindowDarkTickLabelColor() << ini.graphWindowDarkLabelColor()
               << ini.graphWindowDarkGridColor();
    }

    return colors;
}
