#include "graphwindow.h"

#include <QApplication>
#include <QDateTime>
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

GraphWindow::GraphWindow(int graphId, QWidget *parent) :
    FormWindow(parent), m_iniGraph(Fort::iniUser(), graphId)
{
    setupUi();
    setupFlagsAndColors();
    setupTimer();

    setupFormWindow(iniUser());

    connect(confManager(), &ConfManager::iniUserChanged, this, &GraphWindow::updateWindowGeometry);

    connect(this, &GraphWindow::mouseRightClick, this, [&](QMouseEvent *event) {
        windowManager()->trayIcon()->showTrayMenu(GuiUtil::globalPos(event));
    });

    connect(statManager(), &StatManager::trafficAdded, this, &GraphWindow::addTraffic);
}

QString GraphWindow::iniGroup() const
{
    return iniGraph().group();
}

bool GraphWindow::deleteOnClose() const
{
    return !iniGraph().hideOnClose();
}

void GraphWindow::saveWindowState(bool wasVisible)
{
    iniGraph().setVisible(wasVisible);

    saveWindowGeometry();
}

void GraphWindow::restoreWindowState()
{
    const IniGraph &ini = iniGraph();

    m_iniGeometry = ini.geometry();
    m_iniMaximized = ini.maximized();

    stateWatcher()->restore(this, QSize(400, 300), m_iniGeometry, m_iniMaximized);
}

void GraphWindow::saveWindowGeometry()
{
    IniGraph &ini = iniGraph();

    m_iniGeometry = stateWatcher()->geometry();
    m_iniMaximized = stateWatcher()->maximized();

    ini.setGeometry(m_iniGeometry);
    ini.setMaximized(m_iniMaximized);

    confManager()->saveIniUser();
}

void GraphWindow::updateWindowGeometry()
{
    const IniGraph &ini = iniGraph();

    if (ini.geometry() == m_iniGeometry && ini.maximized() == m_iniMaximized)
        return;

    restoreWindowState();
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

    const IniGraph &ini = iniGraph();

    updateWindowFlags(ini);
    updateColors(ini);
    updateFonts(ini);
    updateFormat(ini);

    updateGraph();
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

void GraphWindow::updateColors(const IniGraph &ini)
{
    setWindowOpacityPercent(ini.opacity());

    m_plot->setColors(getColors(ini));
}

void GraphWindow::updateFonts(const IniGraph &ini)
{
    m_plot->setTickLabelSize(ini.tickLabelSize());
    m_plot->setSpeedTextSize(ini.speedTextSize());
    m_plot->setSpeedVisible(ini.showSpeed());
}

void GraphWindow::updateFormat(const IniGraph &ini)
{
    m_plot->setGraphVisible(ini.showGraph());
    m_plot->setMaxSeconds(ini.maxSeconds());
    m_plot->setGraphType(GraphPlot::graphTypeByIndex(ini.graphType()));
    m_plot->setTotalVisible(ini.showTotal());
    m_plot->setInVisible(ini.showIn());
    m_plot->setOutVisible(ini.showOut());
    m_plot->setBarWidth(ini.barWidth());
    m_plot->setAxisTicksVisible(ini.axisTicks());
    m_plot->setYAxisLabelsVisible(!ini.hideYAxisLabels());
    m_plot->setBorderVisible(ini.frameless());
    m_plot->setUnitFormat(FormatUtil::graphUnitFormat(ini.trafUnit()));
    m_plot->setFixedValueMax(ini.fixedSpeed() * 1024LL);
    m_plot->setAnimated(ini.animation());
}

void GraphWindow::setupTimer()
{
    connect(&m_hoverTimer, &QTimer::timeout, this, &GraphWindow::checkHoverLeave);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphWindow::updateGraph);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphWindow::startUpdateTimer);

    m_hoverTimer.setInterval(300);

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setTimerType(Qt::PreciseTimer);

    startUpdateTimer();
}

void GraphWindow::startUpdateTimer()
{
    // Update right after the next second's start
    const int msecs = int(QDateTime::currentMSecsSinceEpoch() % 1000);

    m_updateTimer.start(1000 - msecs + 10);
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

    if (iniGraph().hideOnHover()) {
        hide();
        m_hoverTimer.start();
        return;
    }

    setWindowOpacityPercent(iniGraph().hoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

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
    m_plot->addPoint({ unixTime, inBytes * 8, outBytes * 8 });
}

void GraphWindow::updateGraph()
{
    const qint64 unixTime = DateUtil::getUnixTime();

    // Move the graph to the current time
    addTraffic(unixTime, 0, 0);

    if (!isVisible())
        return;

    updateSpeed(unixTime - 1); // the last complete second

    m_plot->replot();
}

void GraphWindow::updateSpeed(qint64 unixTime)
{
    const bool showTextSpeed = m_plot->speedVisible();
    const bool showWindowSpeed = (windowFlags() & Qt::FramelessWindowHint) == 0;

    if (!(showTextSpeed || showWindowSpeed))
        return;

    const GraphPoint point = m_plot->pointAt(unixTime);
    const auto unitFormat = m_plot->unitFormat();

    const QString inText = FormatUtil::formatSpeed(qint64(point.inBits), unitFormat);
    const QString outText = FormatUtil::formatSpeed(qint64(point.outBits), unitFormat);

    if (showTextSpeed) {
        m_plot->setSpeedText(inText, outText);
    }

    if (showWindowSpeed) {
        setWindowTitle(inArrow + inText + "  " + outArrow + outText);
    }
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

GraphPlot::ColorArray GraphWindow::getColors(const IniGraph &ini)
{
    GraphPlot::ColorArray colors;

    const bool isLightTheme =
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QApplication::styleHints()->colorScheme() != Qt::ColorScheme::Dark;
#else
            true;
#endif

    for (int i = 0; i < GraphColorCount; ++i) {
        colors << ini.color(GraphColorType(i), /*dark=*/!isLightTheme);
    }

    return colors;
}
