#include "graphwindow.h"

#include <QGuiApplication>
#include <QVBoxLayout>

#include "../../fortsettings.h"
#include "../../util/dateutil.h"
#include "../../util/net/netutil.h"
#include "axistickerspeed.h"
#include "graphplot.h"

GraphWindow::GraphWindow(FortSettings *fortSettings, QWidget *parent) :
    WidgetWindow(parent), m_fortSettings(fortSettings)
{
    setupUi();
    setupTimer();

    updateWindowFlags();
    updateColors();

    setMinimumSize(QSize(100, 50));
}

void GraphWindow::updateWindowFlags()
{
    const bool visible = isVisible();

    setWindowFlags(Qt::Tool | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
            | (m_fortSettings->graphWindowAlwaysOnTop() ? Qt::WindowStaysOnTopHint : Qt::Widget)
            | (m_fortSettings->graphWindowFrameless() ? Qt::FramelessWindowHint : Qt::Widget)
            | (m_fortSettings->graphWindowClickThrough() ? Qt::WindowTransparentForInput
                                                         : Qt::Widget));

    if (visible) {
        show(); // setWindowFlags() hides the window
    }
}

void GraphWindow::updateColors()
{
    setWindowOpacityPercent(m_fortSettings->graphWindowOpacity());

    m_plot->setBackground(QBrush(m_fortSettings->graphWindowColor()));

    // Axis
    auto yAxis = m_plot->yAxis;

    const QColor axisColor = m_fortSettings->graphWindowAxisColor();
    yAxis->setBasePen(adjustPen(yAxis->basePen(), axisColor));
    yAxis->setTickPen(adjustPen(yAxis->tickPen(), axisColor));
    yAxis->setSubTickPen(adjustPen(yAxis->subTickPen(), axisColor));

    yAxis->setTickLabelColor(m_fortSettings->graphWindowTickLabelColor());
    yAxis->setLabelColor(m_fortSettings->graphWindowLabelColor());

    yAxis->grid()->setPen(adjustPen(yAxis->grid()->pen(), m_fortSettings->graphWindowGridColor()));

    // Graph Inbound
    m_graphIn->setPen(QPen(m_fortSettings->graphWindowColorIn()));

    // Graph Outbound
    m_graphOut->setPen(QPen(m_fortSettings->graphWindowColorOut()));
}

void GraphWindow::setupUi()
{
    m_plot = new GraphPlot(this);
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
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_plot);
    setLayout(mainLayout);
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
    m_mousePressOffset = event->globalPos() - pos();

    QGuiApplication::setOverrideCursor(Qt::SizeAllCursor);
}

void GraphWindow::onMouseDragMove(QMouseEvent *event)
{
    if (isMaximized() || isFullScreen())
        return;

    move(event->globalPos() - m_mousePressOffset);
}

void GraphWindow::onMouseDragEnd(QMouseEvent *event)
{
    Q_UNUSED(event);

    QGuiApplication::restoreOverrideCursor();
}

void GraphWindow::enterEvent(
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QEvent *event
#else
        QEnterEvent *event
#endif
)
{
    Q_UNUSED(event);

    if (m_fortSettings->graphWindowHideOnHover()) {
        hide();
        m_hoverTimer.start(200);
        return;
    }

    setWindowOpacityPercent(m_fortSettings->graphWindowHoverOpacity());
}

void GraphWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);

    setWindowOpacityPercent(m_fortSettings->graphWindowOpacity());
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

    const qint64 rangeLower = unixTime - m_fortSettings->graphWindowMaxSeconds();

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

QPen GraphWindow::adjustPen(const QPen &pen, const QColor &color)
{
    QPen newPen(pen);
    newPen.setColor(color);
    return newPen;
}
