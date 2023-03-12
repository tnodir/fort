#include "graphwindow.h"

#include <QGuiApplication>
#include <QScreen>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <fortcompat.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>
#include <util/net/netutil.h>
#include <util/window/widgetwindowstatewatcher.h>

#include "axistickerspeed.h"
#include "graphplot.h"

GraphWindow::GraphWindow(QWidget *parent) :
    WidgetWindow(parent), m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupFlagsAndColors();
    setupTimer();
    setupStateWatcher();

    setMinimumSize(QSize(30, 10));
}

ConfManager *GraphWindow::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *GraphWindow::conf() const
{
    return confManager()->conf();
}

IniOptions *GraphWindow::ini() const
{
    return &conf()->ini();
}

IniUser *GraphWindow::iniUser() const
{
    return confManager()->iniUser();
}

void GraphWindow::saveWindowState(bool wasVisible)
{
    iniUser()->setGraphWindowGeometry(m_stateWatcher->geometry());
    iniUser()->setGraphWindowMaximized(m_stateWatcher->maximized());

    iniUser()->setGraphWindowVisible(wasVisible);

    confManager()->saveIniUser();
}

void GraphWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(400, 300), iniUser()->graphWindowGeometry(),
            iniUser()->graphWindowMaximized());
}

void GraphWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
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
    m_plot->xAxis->setVisible(false);

    auto yAxis = m_plot->yAxis;
    yAxis->setVisible(true);
    yAxis->setPadding(1);
    yAxis->setTickLabelPadding(2);

    // Axis Rect
    auto axisRect = m_plot->axisRect();
    axisRect->setMinimumMargins(QMargins(1, 1, 1, 1));

    // Axis Ticker
    QSharedPointer<AxisTickerSpeed> ticker(new AxisTickerSpeed());
    yAxis->setTicker(ticker);

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
    auto mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_plot);
    setLayout(mainLayout);
}

void GraphWindow::setupFlagsAndColors()
{
    const auto updateFlagsAndColors = [&] {
        updateWindowFlags();
        updateColors();
    };

    updateFlagsAndColors();

    connect(confManager(), &ConfManager::confChanged, this, updateFlagsAndColors);
}

void GraphWindow::updateWindowFlags()
{
    const bool visible = isVisible();

    setWindowFlags(Qt::Tool | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
            | (ini()->graphWindowAlwaysOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget)
            | (ini()->graphWindowFrameless() ? Qt::FramelessWindowHint : Qt::Widget)
            | (ini()->graphWindowClickThrough() ? Qt::WindowTransparentForInput : Qt::Widget));

    if (visible) {
        show(); // setWindowFlags() hides the window
    }
}

void GraphWindow::updateColors()
{
    setWindowOpacityPercent(ini()->graphWindowOpacity());

    m_plot->setBackground(QBrush(ini()->graphWindowColor()));

    // Axis
    auto yAxis = m_plot->yAxis;

    const QColor axisColor = ini()->graphWindowAxisColor();
    yAxis->setBasePen(adjustPen(yAxis->basePen(), axisColor));
    yAxis->setTickPen(adjustPen(yAxis->tickPen(), axisColor));
    yAxis->setSubTickPen(adjustPen(yAxis->subTickPen(), axisColor));

    yAxis->setTickLabelColor(ini()->graphWindowTickLabelColor());
    yAxis->setLabelColor(ini()->graphWindowLabelColor());

    yAxis->grid()->setPen(adjustPen(yAxis->grid()->pen(), ini()->graphWindowGridColor()));

    // Graph Inbound
    m_graphIn->setPen(QPen(ini()->graphWindowColorIn()));

    // Graph Outbound
    m_graphOut->setPen(QPen(ini()->graphWindowColorOut()));
}

void GraphWindow::setupTimer()
{
    connect(&m_hoverTimer, &QTimer::timeout, this, &GraphWindow::checkHoverLeave);
    connect(&m_updateTimer, &QTimer::timeout, this, &GraphWindow::addEmptyTraffic);

    m_updateTimer.start(1000); // 1 second
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
    m_mousePressPoint = mouseEventGlobalPos(event);
    m_posOnMousePress = this->pos();
    m_sizeOnMousePress = this->size();
    m_mouseDragResize = (event->buttons() & Qt::RightButton) != 0;

    QGuiApplication::setOverrideCursor(m_mouseDragResize ? Qt::SizeFDiagCursor : Qt::SizeAllCursor);
}

void GraphWindow::onMouseDragMove(QMouseEvent *event)
{
    if (isMaximized() || isFullScreen())
        return;

    const QPoint offset = mouseEventGlobalPos(event) - m_mousePressPoint;

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

    if (ini()->graphWindowHideOnHover()) {
        hide();
        m_hoverTimer.start(200);
        return;
    }

    setWindowOpacityPercent(ini()->graphWindowHoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    setWindowOpacityPercent(ini()->graphWindowOpacity());
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

    const qint64 rangeLower = unixTime - ini()->graphWindowMaxSeconds();

    addData(m_graphIn, rangeLower, unixTime, inBytes);
    addData(m_graphOut, rangeLower, unixTime, outBytes);

    m_plot->xAxis->setRange(unixTime, qFloor(m_plot->axisRect()->width() / 4), Qt::AlignRight);

    m_graphIn->rescaleValueAxis(false, true);
    m_graphOut->rescaleValueAxis(true, true);

    // Avoid negative Y range
    QCPRange yRange = m_plot->yAxis->range();
    if (yRange.lower < 0) {
        yRange.upper -= yRange.lower;
        yRange.lower = 0;

        m_plot->yAxis->setRange(yRange);
    }

    m_plot->replot();
}

void GraphWindow::addEmptyTraffic()
{
    addTraffic(DateUtil::getUnixTime(), 0, 0);
}

void GraphWindow::addData(QCPBars *graph, qint64 rangeLower, qint64 unixTime, quint32 bytes)
{
    auto data = graph->data();

    if (!data->isEmpty()) {
        // Remove old keys
        auto lo = data->constBegin();
        if (lo->mainKey() < rangeLower) {
            data->removeBefore(rangeLower);
        }

        // Check existing key
        auto hi = data->constEnd() - 1;
        if (qFuzzyCompare(unixTime, hi->mainKey())) {
            if (bytes == 0)
                return;

            bytes += quint32(hi->mainValue());

            data->removeAfter(unixTime);
        } else if (unixTime < hi->mainKey()) {
            data->clear();
        }
    }

    // Add data
    data->add(QCPBarsData(unixTime, bytes));
}

void GraphWindow::updateWindowTitleSpeed()
{
    if (windowFlags() & Qt::FramelessWindowHint)
        return;

    const auto inBytes =
            m_graphIn->data()->isEmpty() ? 0 : (m_graphIn->data()->constEnd() - 1)->mainValue();
    const auto outBytes =
            m_graphOut->data()->isEmpty() ? 0 : (m_graphOut->data()->constEnd() - 1)->mainValue();

    setWindowTitle(QChar(0x2193) // ↓
            + NetUtil::formatSpeed(quint32(inBytes)) + QLatin1Char(' ') + QChar(0x2191) // ↑
            + NetUtil::formatSpeed(quint32(outBytes)));
}

void GraphWindow::setWindowOpacityPercent(int percent)
{
    setWindowOpacity(qreal(qBound(1, percent, 100)) / 100.0);
}

void GraphWindow::checkWindowEdges()
{
    constexpr int stickyDistance = 30;

    const auto screen = this->screen();
    if (!screen)
        return;

    const QRect sg = screen->geometry();
    const QRect wg = this->frameGeometry();
    QPoint diff(0, 0);

    const int leftDiff = sg.x() - wg.x();
    if (qAbs(leftDiff) < stickyDistance) {
        diff.setX(leftDiff);
    } else {
        const int rightDiff = sg.width() - wg.right();
        if (qAbs(rightDiff) < stickyDistance) {
            diff.setX(rightDiff);
        }
    }

    const int topDiff = sg.y() - wg.y();
    if (qAbs(topDiff) < stickyDistance) {
        diff.setY(topDiff);
    } else {
        const int bottomDiff = sg.height() - wg.bottom();
        if (qAbs(bottomDiff) < stickyDistance) {
            diff.setY(bottomDiff);
        }
    }

    if (diff.x() != 0 || diff.y() != 0) {
        this->move(wg.x() + diff.x(), wg.y() + diff.y());
    }
}

QPen GraphWindow::adjustPen(const QPen &pen, const QColor &color)
{
    QPen newPen(pen);
    newPen.setColor(color);
    return newPen;
}
