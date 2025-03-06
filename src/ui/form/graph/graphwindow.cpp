#include "graphwindow.h"

#include <QApplication>
#include <QScreen>
#include <QStyleHints>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <form/controls/controlutil.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/guiutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "axistickerspeed.h"
#include "graphplot.h"

namespace {

constexpr int stickyDistance = 30;

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

bool clearGraphData(
        const QSharedPointer<QCPBarsDataContainer> &data, double rangeLowerKey, double unixTimeKey)
{
    if (data->isEmpty())
        return true;

    const auto hi = data->constEnd() - 1;
    if (rangeLowerKey > hi->mainKey() || unixTimeKey < hi->mainKey()) {
        data->clear();
        return true;
    }

    const auto lo = data->constBegin();
    if (lo->mainKey() < rangeLowerKey) {
        data->removeBefore(rangeLowerKey);
    }

    return data->isEmpty();
}

void adjustGraphData(
        const QSharedPointer<QCPBarsDataContainer> &data, double unixTimeKey, quint32 &bits)
{
    const auto hi = data->constEnd() - 1;

    // Check existing key
    if (qFuzzyCompare(unixTimeKey, hi->mainKey())) {
        bits += quint32(hi->mainValue());
    }

    data->removeAfter(unixTimeKey);
}

QPen adjustPen(const QPen &pen, const QColor &color)
{
    QPen newPen(pen);
    newPen.setColor(color);
    return newPen;
}

}

GraphWindow::GraphWindow(QWidget *parent) : FormWindow(parent)
{
    setupUi();
    setupFlagsAndColors();
    setupTimer();

    setupFormWindow(iniUser(), IniUser::graphWindowGroup());
}

bool GraphWindow::deleteOnClose() const
{
    return !iniUser()->graphWindowHideOnClose();
}

ConfManager *GraphWindow::confManager() const
{
    return IoC<ConfManager>();
}

IniUser *GraphWindow::iniUser() const
{
    return &confManager()->iniUser();
}

void GraphWindow::saveWindowState(bool wasVisible)
{
    iniUser()->setGraphWindowGeometry(stateWatcher()->geometry());
    iniUser()->setGraphWindowMaximized(stateWatcher()->maximized());

    iniUser()->setGraphWindowVisible(wasVisible);

    confManager()->saveIniUser();
}

void GraphWindow::restoreWindowState()
{
    stateWatcher()->restore(this, QSize(400, 300), iniUser()->graphWindowGeometry(),
            iniUser()->graphWindowMaximized());
}

void GraphWindow::setupUi()
{
    m_plot = new GraphPlot();
    m_plot->setContentsMargins(0, 0, 0, 0);

    // Interactions
    connect(m_plot, &GraphPlot::resized, this, &GraphWindow::addEmptyTraffic);

    connect(m_plot, &GraphPlot::mouseDoubleClick, this, &GraphWindow::onMouseDoubleClick);
    connect(m_plot, &GraphPlot::mouseRightClick, this, &GraphWindow::mouseRightClick);

    connect(m_plot, &GraphPlot::mouseDragBegin, this, &GraphWindow::onMouseDragBegin);
    connect(m_plot, &GraphPlot::mouseDragMove, this, &GraphWindow::onMouseDragMove);
    connect(m_plot, &GraphPlot::mouseDragEnd, this, &GraphWindow::onMouseDragEnd);

    // Axis
    auto xAxis = m_plot->xAxis;
    xAxis->setVisible(false);

    auto yAxis = m_plot->yAxis;
    yAxis->setVisible(true);
    yAxis->setTickLabelPadding(2);

    // Axis Rect
    auto axisRect = m_plot->axisRect();
    axisRect->setMinimumMargins(QMargins(1, 2, 1, 1));

    // Axis Ticker
    m_ticker.reset(new AxisTickerSpeed());
    yAxis->setTicker(m_ticker);

    // Graph Inbound
    m_graphIn = new QCPBars(m_plot->xAxis, m_plot->yAxis);
    m_graphIn->setAntialiased(false);
    m_graphIn->setWidthType(QCPBars::wtAbsolute);
    m_graphIn->setWidth(1);

    // Graph Outbound
    m_graphOut = new QCPBars(m_plot->xAxis, m_plot->yAxis);
    m_graphOut->setAntialiased(false);
    m_graphOut->setWidthType(QCPBars::wtAbsolute);
    m_graphOut->setWidth(1);

    // Bars Group
    auto group = new QCPBarsGroup(m_plot);
    group->setSpacing(1);
    group->append(m_graphIn);
    group->append(m_graphOut);

    // Widget Layout
    auto layout = ControlUtil::createVLayout();
    layout->addWidget(m_plot);
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
    updateFlagsAndColors(*iniUser(), /*onlyFlags=*/false);
}

void GraphWindow::updateFlagsAndColors(const IniUser &ini, bool onlyFlags)
{
    if (onlyFlags)
        return;

    updateWindowFlags(ini);
    updateColors(ini);
    updateFormat(ini);
}

void GraphWindow::updateWindowFlags(const IniUser &ini)
{
    const bool visible = isVisible();

    setWindowFlags(Qt::Tool | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
            | (ini.graphWindowAlwaysOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget)
            | (ini.graphWindowFrameless() ? Qt::FramelessWindowHint : Qt::Widget)
            | (ini.graphWindowClickThrough() ? Qt::WindowTransparentForInput : Qt::Widget));

    if (visible) {
        show(); // setWindowFlags() hides the window
    }
}

void GraphWindow::updateColors(const IniUser &ini)
{
    const auto colors = getColors(ini);

    setWindowOpacityPercent(ini.graphWindowOpacity());

    // Background Color
    {
        const auto bgColor = colors[ColorBg];
        const bool isTransparentBg = (bgColor == Qt::transparent);

        m_plot->setBackground(isTransparentBg ? Qt::NoBrush : QBrush(bgColor));
    }

    // Axis
    auto yAxis = m_plot->yAxis;

    const QColor axisColor = colors[ColorAxis];
    yAxis->setBasePen(adjustPen(yAxis->basePen(), axisColor));
    yAxis->setTickPen(adjustPen(yAxis->tickPen(), axisColor));
    yAxis->setSubTickPen(adjustPen(yAxis->subTickPen(), axisColor));

    yAxis->setTickLabelColor(colors[ColorTickLabel]);
    yAxis->setLabelColor(colors[ColorLabel]);

    yAxis->grid()->setPen(adjustPen(yAxis->grid()->pen(), colors[ColorGrid]));

    // Graph Inbound
    m_graphIn->setPen(QPen(colors[ColorIn]));

    // Graph Outbound
    m_graphOut->setPen(QPen(colors[ColorOut]));
}

void GraphWindow::updateFormat(const IniUser &ini)
{
    m_unitFormat = FormatUtil::graphUnitFormat(ini.graphWindowTrafUnit());

    m_ticker->setUnitFormat(m_unitFormat);
}

void GraphWindow::setupTimer()
{
    connect(&m_hoverTimer, &QTimer::timeout, this, &GraphWindow::checkHoverLeave);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphWindow::addEmptyTraffic);

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

void GraphWindow::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);

    if (iniUser()->graphWindowHideOnHover()) {
        hide();
        m_hoverTimer.start();
        return;
    }

    setWindowOpacityPercent(iniUser()->graphWindowHoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    setWindowOpacityPercent(iniUser()->graphWindowOpacity());
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

void GraphWindow::addTraffic(qint64 unixTime, quint32 inBytes, quint32 outBytes)
{
    if (m_lastUnixTime != unixTime) {
        m_lastUnixTime = unixTime;

        updateWindowTitleSpeed();
    }

    const qint64 rangeLower = unixTime - iniUser()->graphWindowMaxSeconds();

    const double rangeLowerKey = double(rangeLower);
    const double unixTimeKey = double(unixTime);

    addData(m_graphIn, rangeLowerKey, unixTimeKey, inBytes);
    addData(m_graphOut, rangeLowerKey, unixTimeKey, outBytes);

    m_plot->xAxis->setRange(unixTimeKey, qFloor(m_plot->axisRect()->width() / 4), Qt::AlignRight);

    m_graphIn->rescaleValueAxis(false, true);
    m_graphOut->rescaleValueAxis(true, true);

    // Avoid negative Y range
    QCPRange yRange = m_plot->yAxis->range();
    if (yRange.lower < 0) {
        yRange.upper -= yRange.lower;
        yRange.lower = 0;

        m_plot->yAxis->setRange(yRange);
    }

    const qint64 yRangeMax = iniUser()->graphWindowFixedSpeed() * 1024LL;
    if (yRangeMax > 0) {
        yRange.upper = yRangeMax;

        m_plot->yAxis->setRange(yRange);
    }

    m_plot->replot();
}

void GraphWindow::addEmptyTraffic()
{
    addTraffic(DateUtil::getUnixTime(), 0, 0);
}

void GraphWindow::addData(QCPBars *graph, double rangeLowerKey, double unixTimeKey, quint32 bytes)
{
    auto data = graph->data();
    quint32 bits = bytes * 8;

    if (!clearGraphData(data, rangeLowerKey, unixTimeKey)) {
        adjustGraphData(data, unixTimeKey, bits);
    }

    // Add data
    data->add(QCPBarsData(unixTimeKey, bits));
}

void GraphWindow::updateWindowTitleSpeed()
{
    if (windowFlags() & Qt::FramelessWindowHint)
        return;

    const auto inBits =
            m_graphIn->data()->isEmpty() ? 0 : (m_graphIn->data()->constEnd() - 1)->mainValue();
    const auto outBits =
            m_graphOut->data()->isEmpty() ? 0 : (m_graphOut->data()->constEnd() - 1)->mainValue();

    setWindowTitle(QChar(0x2193) // ↓
            + FormatUtil::formatSpeed(quint64(inBits), m_unitFormat) + ' ' + QChar(0x2191) // ↑
            + FormatUtil::formatSpeed(quint64(outBits), m_unitFormat));
}

void GraphWindow::setWindowOpacityPercent(int percent)
{
    setWindowOpacity(qreal(qBound(1, percent, 100)) / 100.0);
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

QVarLengthArray<QColor, GraphWindow::ColorCount> GraphWindow::getColors(const IniUser &ini)
{
    QVarLengthArray<QColor, GraphWindow::ColorCount> colors;

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
