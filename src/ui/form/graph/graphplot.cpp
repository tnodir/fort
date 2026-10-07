#include "graphplot.h"

#include <QDateTime>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "graphbarsitem.h"
#include "graphlineitem.h"

namespace {

inline constexpr int axisPadding = 2;
inline constexpr int axisMarginTop = 2;
inline constexpr int tickLength = 5;
inline constexpr int subTickLength = 2;
inline constexpr int tickLabelPadding = 4;
inline constexpr int unitLabelPadding = 6;
inline constexpr int risingMsecs = 400;
inline constexpr int scaleMsecs = 400;
inline constexpr qreal valueHeadroom = 1.1f; // the empty space above the highest bar
inline constexpr int speedBgAlpha = 130;
inline constexpr int barOutlineDarker = 150; // as in NetTraffic
inline constexpr qreal lineCenterOffset = 1.5; // device pixels: a line of 2 is above the value
inline constexpr int speedArrowSpacing = 2;
inline constexpr double speedArrowTop = 0.18; // of the text's height
inline constexpr double speedArrowWidth = 0.55; // of the text's height
inline constexpr double speedArrowHeight = 0.66; // of the text's height
inline constexpr double speedArrowShaft = 0.36; // of the arrow's width
inline constexpr double speedArrowHead = 0.45; // of the arrow's height
inline constexpr int gridLineWidth = 2;
inline constexpr int mouseMoveDistance = 3;

void addHLine(QPainterPath &path, int x1, int x2, int y)
{
    path.moveTo(x1, y);
    path.lineTo(x2, y);
}

void addVLine(QPainterPath &path, int x, int y1, int y2)
{
    path.moveTo(x, y1);
    path.lineTo(x, y2);
}

void addHLines(QPainterPath &path, const QVector<int> &ys, const QRect &axisRect)
{
    const int bottom = axisRect.bottom();

    for (const int y : ys) {
        // No line at the zero
        if (y < bottom) {
            addHLine(path, axisRect.left(), axisRect.right(), y);
        }
    }
}

QPen gridPen(const QColor &color)
{
    QPen pen(color, gridLineWidth);
    pen.setCosmetic(true);
    pen.setDashPattern({ 2, 1 }); // in the line widths: as Qt::DashLine of 1 pixel's width
    pen.setCapStyle(Qt::FlatCap); // the square caps fill the gaps

    return pen;
}

void addTicks(QPainterPath &path, const QVector<int> &ys, const QRect &axisRect, int length)
{
    const int left = axisRect.left();
    const int right = axisRect.right() + 1;

    for (const int y : ys) {
        addHLine(path, left - length, left, y);
        addHLine(path, right, right + length, y);
    }
}

QPainterPath arrowPath(const QRectF &rect, bool down)
{
    const qreal shaftHalf = rect.width() * speedArrowShaft / 2;
    const qreal headHeight = rect.height() * speedArrowHead;
    const qreal centerX = rect.center().x();

    // From the shaft's end to the head's tip
    const qreal endY = down ? rect.top() : rect.bottom();
    const qreal tipY = down ? rect.bottom() : rect.top();
    const qreal headY = down ? (tipY - headHeight) : (tipY + headHeight);

    const QPolygonF polygon = {
        { centerX - shaftHalf, endY },
        { centerX + shaftHalf, endY },
        { centerX + shaftHalf, headY },
        { rect.right(), headY },
        { centerX, tipY },
        { rect.left(), headY },
        { centerX - shaftHalf, headY },
    };

    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();

    return path;
}

qreal layoutSpeedItem(QGraphicsItem *item, qreal x)
{
    item->setPos(x, 0);

    // The items' rectangles start at 0
    return x + item->boundingRect().right();
}

// Smooth, unlike the other items aligned to the pixels
class SmoothPathItem : public QGraphicsPathItem
{
public:
    using QGraphicsPathItem::QGraphicsPathItem;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override
    {
        const bool antialiased = painter->testRenderHint(QPainter::Antialiasing);

        painter->setRenderHint(QPainter::Antialiasing);
        QGraphicsPathItem::paint(painter, option, widget);
        painter->setRenderHint(QPainter::Antialiasing, antialiased);
    }
};

template<typename T>
T *createNoPenItem(QGraphicsItem *parent)
{
    auto item = new T(parent);
    item->setPen(Qt::NoPen);

    return item;
}

int risingHeight(int height, double ratio)
{
    return (height > 0) ? qMax(qRound(height * ratio), 1) : 0;
}

GraphColumn risingColumn(const GraphColumn &column, double ratio)
{
    GraphColumn rising = column;
    rising.inHeight = risingHeight(column.inHeight, ratio);
    rising.outHeight = risingHeight(column.outHeight, ratio);
    rising.totalHeight = risingHeight(column.totalHeight, ratio);

    return rising;
}

void appendBar(QVector<QRectF> &rects, const QRectF &rect)
{
    if (!rect.isEmpty()) {
        rects.append(rect);
    }
}

void setBarsColor(GraphBarsItem *item, const QColor &color)
{
    item->setBrush(color);
    item->setOutlineBrush(color.darker(barOutlineDarker));
}

void appendColumn(GraphBarsRects &bars, const GraphColumnRects &rects)
{
    appendBar(bars.in, rects.in);
    appendBar(bars.out, rects.out);
    appendBar(bars.total, rects.total);
}

void setRisingBar(GraphBarsItem *item, const QRectF &rect, const QVector<QRectF> &neighbors)
{
    QVector<QRectF> rects;
    appendBar(rects, rect);

    item->setRects(rects, neighbors);
}

}

GraphPlot::GraphPlot(QWidget *parent) : QGraphicsView(parent)
{
    setupView();
    setupItems();
    setupScrollTimer();
    setupRisingAnimation();
    setupScaleAnimation();
}

void GraphPlot::setAnimated(bool v)
{
    m_animated = v;

    m_risingAnimation.setEnabled(v);
    m_scaleAnimation.setEnabled(v);
}

void GraphPlot::setGraphType(GraphType v)
{
    if (m_graphType == v)
        return;

    m_graphType = v;

    updateItemsVisible();
}

void GraphPlot::setTotalVisible(bool v)
{
    m_totalVisible = v;

    updateItemsVisible();
}

void GraphPlot::setInVisible(bool v)
{
    m_inVisible = v;

    updateItemsVisible();
}

void GraphPlot::setOutVisible(bool v)
{
    m_outVisible = v;

    updateItemsVisible();
}

void GraphPlot::setAxisTicksVisible(bool v)
{
    if (m_axisTicksVisible == v)
        return;

    m_axisTicksVisible = v;

    m_axesChanged = true;
}

void GraphPlot::setBorderVisible(bool v)
{
    if (m_borderVisible == v)
        return;

    m_borderVisible = v;

    m_axesChanged = true;
}

void GraphPlot::setUnitFormat(FormatUtil::SizeFormat v)
{
    m_ticker.setUnitFormat(v);

    m_axesChanged = true; // the tick labels are changed
}

bool GraphPlot::speedVisible() const
{
    return m_speedBox->isVisible();
}

void GraphPlot::setSpeedVisible(bool v)
{
    m_speedBox->setVisible(v);
}

void GraphPlot::setSpeedText(const QString &inText, const QString &outText)
{
    m_speedInText->setText(inText);
    m_speedOutText->setText(outText);
}

GraphPoint GraphPlot::pointAt(qint64 unixTime) const
{
    const int index = pointIndex(unixTime);

    return isPointAt(index, unixTime) ? m_points.at(index) : GraphPoint();
}

void GraphPlot::setColors(const ColorArray &colors)
{
    // Background Color
    const QColor bgColor = colors[ColorBg];
    const bool isTransparentBg = (bgColor == Qt::transparent);

    setBackgroundBrush(isTransparentBg ? QBrush(Qt::NoBrush) : QBrush(bgColor));

    // Grid
    m_grid->setPen(gridPen(colors[ColorGrid]));

    // Graph Inbound
    setBarsColor(m_barsIn, colors[ColorIn]);
    setBarsColor(m_risingIn, colors[ColorIn]);

    // Graph Outbound
    setBarsColor(m_barsOut, colors[ColorOut]);
    setBarsColor(m_risingOut, colors[ColorOut]);

    // Graph Total
    setBarsColor(m_barsTotal, colors[ColorTotal]);
    setBarsColor(m_risingTotal, colors[ColorTotal]);

    // Graph Lines
    m_lineTotal->setColor(colors[ColorTotal]);
    m_fillIn->setColor(colors[ColorIn]);
    m_fillOut->setColor(colors[ColorOut]);
    m_lineIn->setColor(colors[ColorIn]);
    m_lineOut->setColor(colors[ColorOut]);

    // Text Speed
    {
        QColor speedBgColor = bgColor;
        speedBgColor.setAlpha(speedBgAlpha);

        m_speedBox->setBrush(speedBgColor);

        m_speedInArrow->setBrush(colors[ColorIn]);
        m_speedInText->setBrush(colors[ColorLabel]);
        m_speedOutArrow->setBrush(colors[ColorOut]);
        m_speedOutText->setBrush(colors[ColorLabel]);
    }

    // Axis
    m_axes->setPen(QPen(colors[ColorAxis], 0, Qt::SolidLine, Qt::SquareCap));

    // Border
    m_border->setBrush(colors[ColorBorder]);

    m_tickLabelColor = colors[ColorTickLabel];

    updateTickLabelsStyle();
}

void GraphPlot::setTickLabelSize(int pointSize)
{
    m_tickLabelFont = font();
    m_tickLabelFont.setPointSize(pointSize);
    m_tickLabelFont.setWeight(QFont::DemiBold);

    // Text Speed
    QFont speedFont = m_tickLabelFont;
    speedFont.setPointSize(pointSize + 1);

    m_speedInText->setFont(speedFont);
    m_speedOutText->setFont(speedFont);

    m_speedSpacing = QFontMetricsF(speedFont).horizontalAdvance(QLatin1String("  "));

    setupSpeedArrows(speedFont);

    updateTickLabelsStyle();

    m_axesChanged = true; // the tick labels' sizes are changed
}

void GraphPlot::addPoint(const GraphPoint &point)
{
    const qint64 lastTime = lastUnixTime();

    if (point.unixTime > lastTime) {
        removeOldPoints(point.unixTime);
        m_points.append(point);
    } else if (point.unixTime < lastTime - m_maxSeconds) {
        // The clock is moved back too far
        m_points = { point };
    } else {
        // The current or a delayed past second
        mergePoint(point);
    }
}

void GraphPlot::cancelMousePressAndDragging()
{
    m_mousePressed = false;
    m_mouseDragging = false;
}

void GraphPlot::replot()
{
    const quint64 bits = maxBits(keyLower());
    m_barsEmpty = (bits == 0);

    animateValueUpper(targetValueUpper(bits));
    startRising(lastUnixTime() - 1); // the last complete second

    redraw();
    updateScroll();
}

void GraphPlot::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    setSceneRect(viewport()->rect());

    m_axesChanged = true;

    emit resized(event);
}

void GraphPlot::hideEvent(QHideEvent *event)
{
    QGraphicsView::hideEvent(event);

    m_scrollTimer.stop(); // the hidden graph isn't replotted
}

void GraphPlot::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    event->accept();

    m_mousePressed = true;
    m_mouseHasMoved = false;
    m_mousePressPos = event->position().toPoint();
}

void GraphPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    event->accept();

    emit mouseDoubleClick(event);
}

void GraphPlot::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);

    if (!m_mousePressed || !checkMouseHasMoved(event))
        return;

    if (!m_mouseDragging) {
        m_mouseDragging = true;
        emit mouseDragBegin(event);
    }

    emit mouseDragMove(event);
}

void GraphPlot::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);

    if (!m_mousePressed)
        return;

    m_mousePressed = false;

    if (m_mouseDragging) {
        m_mouseDragging = false;
        emit mouseDragEnd(event);
    } else if (event->button() == Qt::RightButton) {
        emit mouseRightClick(event);
    }
}

bool GraphPlot::checkMouseHasMoved(QMouseEvent *event)
{
    if (!m_mouseHasMoved) {
        const QPoint offset = event->position().toPoint() - m_mousePressPos;

        m_mouseHasMoved = (offset.manhattanLength() > mouseMoveDistance);
    }

    return m_mouseHasMoved;
}

void GraphPlot::setupView()
{
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setFocusPolicy(Qt::NoFocus);
    setInteractive(false);

    viewport()->setAutoFillBackground(false);

    auto scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);

    setScene(scene);
}

void GraphPlot::setupItems()
{
    auto scene = this->scene();

    // Grid
    m_grid = scene->addPath(QPainterPath());

    // Plot Area: Clips the graphs and the text speed
    m_plotArea = scene->addRect(QRectF(), Qt::NoPen);
    m_plotArea->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    // Bars: Scrolled together
    m_bars = createNoPenItem<QGraphicsRectItem>(m_plotArea);

    // Graph Inbound
    m_barsIn = new GraphBarsItem(m_bars);
    m_risingIn = new GraphBarsItem(m_barsIn);

    // Graph Outbound
    m_barsOut = new GraphBarsItem(m_bars);
    m_risingOut = new GraphBarsItem(m_barsOut);

    // Graph Total: Over the bars
    m_barsTotal = new GraphBarsItem(m_bars);
    m_risingTotal = new GraphBarsItem(m_barsTotal);

    // The scrolled bars aren't repainted
    for (auto item : { m_barsIn, m_barsOut, m_barsTotal }) {
        item->setCached(true);
    }

    // Graph Lines: The total is below, with the area's fill
    m_lineTotal = new GraphLineItem(m_bars);
    m_lineTotal->setFillVisible(true);

    // Without the total: the in and out areas' fills are below both lines
    m_fillIn = new GraphLineItem(m_bars);
    m_fillOut = new GraphLineItem(m_bars);

    for (auto item : { m_fillIn, m_fillOut }) {
        item->setLineVisible(false);
        item->setFillVisible(true);
    }

    m_lineIn = new GraphLineItem(m_bars);
    m_lineOut = new GraphLineItem(m_bars);

    updateItemsVisible();

    // Text Speed
    m_speedBox = createNoPenItem<QGraphicsRectItem>(m_plotArea);

    m_speedInArrow = createNoPenItem<SmoothPathItem>(m_speedBox);
    m_speedInText = new QGraphicsSimpleTextItem(m_speedBox);
    m_speedOutArrow = createNoPenItem<SmoothPathItem>(m_speedBox);
    m_speedOutText = new QGraphicsSimpleTextItem(m_speedBox);

    // Axis
    m_axes = scene->addPath(QPainterPath());

    // Unit Label: Vertical
    m_unitLabel = scene->addSimpleText(QString());
    m_unitLabel->setRotation(-90);

    // Border: Over all
    m_border = new GraphBarsItem();
    scene->addItem(m_border);

    m_tickLabelFont = font();
}

void GraphPlot::setupScrollTimer()
{
    m_scrollTimer.setSingleShot(true);
    m_scrollTimer.setTimerType(Qt::PreciseTimer);

    connect(&m_scrollTimer, &QTimer::timeout, this, &GraphPlot::updateScroll);
}

void GraphPlot::setupRisingAnimation()
{
    m_risingAnimation.setEnabled(m_animated);
    m_risingAnimation.setDuration(risingMsecs);
    m_risingAnimation.setEasingCurve(QEasingCurve::OutCubic);

    connect(&m_risingAnimation, &GraphAnimation::valueChanged, this, &GraphPlot::updateRisingBars);
}

void GraphPlot::setupScaleAnimation()
{
    m_scaleAnimation.setEnabled(m_animated);
    m_scaleAnimation.setDuration(scaleMsecs);
    m_scaleAnimation.setEasingCurve(QEasingCurve::OutCubic);

    connect(&m_scaleAnimation, &GraphAnimation::valueChanged, this, &GraphPlot::updateScale);
}

void GraphPlot::updateItemsVisible()
{
    // The hidden graphs' bars are empty
    for (auto item : { m_barsIn, m_barsOut, m_barsTotal }) {
        item->setVisible(!isLine());
    }

    updateLinesVisible();
}

void GraphPlot::updateLinesVisible()
{
    const bool isLine = this->isLine();
    const bool isInOutFilled = isLine && !m_totalVisible;

    m_lineTotal->setVisible(isLine && m_totalVisible);
    m_fillIn->setVisible(isInOutFilled && m_inVisible);
    m_fillOut->setVisible(isInOutFilled && m_outVisible);
    m_lineIn->setVisible(isLine && m_inVisible);
    m_lineOut->setVisible(isLine && m_outVisible);
}

int GraphPlot::pointIndex(qint64 unixTime) const
{
    const auto it = std::lower_bound(m_points.cbegin(), m_points.cend(), unixTime,
            [](const GraphPoint &point, qint64 t) { return point.unixTime < t; });

    return int(it - m_points.cbegin());
}

bool GraphPlot::isPointAt(int index, qint64 unixTime) const
{
    return index < m_points.size() && m_points.at(index).unixTime == unixTime;
}

void GraphPlot::removeOldPoints(qint64 unixTime)
{
    const qint64 rangeLower = unixTime - m_maxSeconds;

    m_points.remove(0, pointIndex(rangeLower));
}

void GraphPlot::mergePoint(const GraphPoint &point)
{
    const int index = pointIndex(point.unixTime);

    if (!isPointAt(index, point.unixTime)) {
        m_points.insert(index, point);
        return;
    }

    GraphPoint &oldPoint = m_points[index];
    oldPoint.inBits += point.inBits;
    oldPoint.outBits += point.outBits;
}

qint64 GraphPlot::lastUnixTime() const
{
    return m_points.isEmpty() ? 0 : m_points.constLast().unixTime;
}

int GraphPlot::secondPixels() const
{
    // Device pixels per second: the bars are aligned to them to keep their widths on scroll
    return qMax(qRound(m_barWidth * devicePixelRatioF()), 1);
}

int GraphPlot::keyRangeSize() const
{
    const int width = m_axisRect.isNull() ? viewport()->width() : m_axisRect.width();
    const int widthPixels = qFloor(width * devicePixelRatioF());

    // Include the partially visible second
    return qMax(widthPixels / secondPixels(), 0) + 1;
}

qint64 GraphPlot::keyLower() const
{
    return lastUnixTime() - keyRangeSize();
}

GraphBits GraphPlot::visibleBits(const GraphPoint &point) const
{
    return {
        m_inVisible ? point.inBits : 0,
        m_outVisible ? point.outBits : 0,
        m_totalVisible ? (point.inBits + point.outBits) : 0,
    };
}

quint64 GraphPlot::visibleMaxBits(const GraphPoint &point) const
{
    const GraphBits bits = visibleBits(point);

    return std::max({ bits.in, bits.out, bits.total });
}

quint64 GraphPlot::maxBits(qint64 keyLower) const
{
    quint64 bits = 0;

    for (const auto &point : m_points) {
        if (point.unixTime >= keyLower) {
            bits = qMax(bits, visibleMaxBits(point));
        }
    }

    return bits;
}

double GraphPlot::targetValueUpper(quint64 bits) const
{
    if (m_fixedValueMax > 0)
        return double(m_fixedValueMax);

    // Keep the current range for empty traffic
    return (bits > 0) ? (bits * valueHeadroom) : m_valueTarget;
}

void GraphPlot::animateValueUpper(double v)
{
    if (m_valueTarget == v)
        return;

    m_valueTarget = v;

    // The first layout isn't animated
    if (m_axisRect.isNull()) {
        setValueUpper(v);
        return;
    }

    // Change the scale smoothly from the shown one
    m_scaleAnimation.start(m_valueUpper, v);
}

void GraphPlot::setValueUpper(double v)
{
    if (m_valueUpper == v)
        return;

    m_valueUpper = v;
    m_axesChanged = true;
}

void GraphPlot::updateScale(double value)
{
    setValueUpper(value);

    if (m_axesChanged) {
        redraw();
    }
}

void GraphPlot::redraw()
{
    // The axes depend on the value range, the view's size and the tick labels only
    if (m_axesChanged) {
        m_axesChanged = false;
        layoutAxes();
    }

    updateBars();
    updateRisingBars();
    updateSpeedBox();
}

void GraphPlot::layoutAxes()
{
    AxisTicks axisTicks;
    m_ticker.generate(m_valueUpper, axisTicks);

    m_unitLabel->setText(axisTicks.unit);

    const int tickLabelsWidth = setupTickLabels(axisTicks.labels);
    updateAxisRect(tickLabelsWidth);

    const QVector<int> tickYs = valuesToPixels(axisTicks.ticks);
    const QVector<int> subTickYs = valuesToPixels(axisTicks.subTicks);

    updateGrid(tickYs);
    updateAxes(tickYs, subTickYs);
    updateTickLabels(tickYs);
    updateUnitLabel();
    updateBorder();
}

void GraphPlot::setupTickLabelStyle(QGraphicsSimpleTextItem *label) const
{
    label->setFont(m_tickLabelFont);
    label->setBrush(m_tickLabelColor);
}

void GraphPlot::updateTickLabelsStyle()
{
    for (auto label : std::as_const(m_tickLabels)) {
        setupTickLabelStyle(label);
    }

    setupTickLabelStyle(m_unitLabel);
}

int GraphPlot::setupTickLabels(const QStringList &labels)
{
    const int count = labels.size();

    // Add missing labels
    while (m_tickLabels.size() < count) {
        auto label = scene()->addSimpleText(QString());
        setupTickLabelStyle(label);

        m_tickLabels.append(label);
    }

    // Hide extra labels
    for (int i = count; i < m_tickLabels.size(); ++i) {
        m_tickLabels.at(i)->hide();
    }

    int maxWidth = 0;

    for (int i = 0; i < count; ++i) {
        auto label = m_tickLabels.at(i);
        label->setText(labels.at(i));

        maxWidth = qMax(maxWidth, qCeil(label->boundingRect().width()));
    }

    return maxWidth;
}

void GraphPlot::updateAxisRect(int tickLabelsWidth)
{
    const int border = borderWidth();
    const int ticksWidth = axisTickLength();
    const int leftMargin = border + axisPadding + unitLabelWidth() + ticksWidth + tickLabelPadding
            + tickLabelsWidth;
    // The bars are up to the right edge without the ticks
    const int rightMargin = border + ticksWidth;

    m_axisRect =
            viewport()->rect().adjusted(leftMargin, border + axisMarginTop, -rightMargin, -border);

    // Clip the bars at their bottom in device pixels
    QRectF plotRect = m_axisRect;
    plotRect.setBottom(barsBottom() / devicePixelRatioF());

    m_plotArea->setRect(plotRect);
}

int GraphPlot::axisTickLength() const
{
    return m_axisTicksVisible ? tickLength : 0;
}

int GraphPlot::borderWidth() const
{
    return m_borderVisible ? 1 : 0;
}

int GraphPlot::unitLabelWidth() const
{
    // The vertical label's width is its text's height
    return qCeil(m_unitLabel->boundingRect().height()) + unitLabelPadding;
}

void GraphPlot::updateGrid(const QVector<int> &tickYs)
{
    QPainterPath path;
    addHLines(path, tickYs, m_axisRect);

    m_grid->setPath(path);
}

void GraphPlot::startRising(qint64 unixTime)
{
    if (m_risingTime == unixTime)
        return; // keep rising on resize, options' change

    m_risingTime = unixTime;

    m_risingAnimation.stop();

    // The lines are changed at once: they are repainted entirely
    if (isLine())
        return;

    // Nothing to rise for an empty traffic
    if (visibleMaxBits(pointAt(unixTime)) == 0)
        return;

    m_risingAnimation.start(0.0, 1.0);
}

void GraphPlot::updateBars()
{
    if (isLine()) {
        // A second beyond the left edge keeps the clipped curve's shape on scroll
        updateLines(visibleColumns(keyLower() - 1));
        return;
    }

    const QVector<GraphColumn> columns = visibleColumns(keyLower());

    GraphBarsRects bars;

    m_risingColumn = {};
    m_risingNeighbors = {};

    updateBarsOutline();

    for (const GraphColumn &column : columns) {
        // The rising bars are drawn separately
        if (column.unixTime == m_risingTime) {
            m_risingColumn = column;
            continue;
        }

        const GraphColumnRects rects = columnRects(column);

        // The rising bars are outlined with their neighbors
        if (qAbs(column.unixTime - m_risingTime) == 1) {
            appendColumn(m_risingNeighbors, rects);
        }

        appendColumn(bars, rects);
    }

    m_barsIn->setRects(bars.in);
    m_barsOut->setRects(bars.out);
    m_barsTotal->setRects(bars.total);
}

void GraphPlot::updateBarsOutline()
{
    // A device pixel's outline
    const qreal width = 1 / devicePixelRatioF();

    for (auto item : { m_barsIn, m_barsOut, m_barsTotal, m_risingIn, m_risingOut, m_risingTotal }) {
        item->setOutlineWidth(width);
    }
}

void GraphPlot::updateRisingBars()
{
    // The lines have no rising points
    if (isLine())
        return;

    const double ratio = m_risingAnimation.currentValue();
    const GraphColumnRects rects = columnRects(risingColumn(m_risingColumn, ratio));

    setRisingBar(m_risingIn, rects.in, m_risingNeighbors.in);
    setRisingBar(m_risingOut, rects.out, m_risingNeighbors.out);
    setRisingBar(m_risingTotal, rects.total, m_risingNeighbors.total);
}

void GraphPlot::updateLines(const QVector<GraphColumn> &columns)
{
    // The lines are clipped by the plot area's left edge
    GraphLinePoints points;

    appendLineStart(points, columns);

    for (const GraphColumn &column : columns) {
        appendLinePoints(points, column);
    }

    m_lineIn->setPoints(points.in);
    m_lineOut->setPoints(points.out);
    m_lineTotal->setPoints(points.total);
    m_fillIn->setPoints(points.in);
    m_fillOut->setPoints(points.out);
}

void GraphPlot::appendLineStart(GraphLinePoints &points, const QVector<GraphColumn> &columns) const
{
    // Only the lines started inside the plot, without older points, rise from the zero
    if (columns.isEmpty() || columns.constFirst().unixTime != m_points.constFirst().unixTime)
        return;

    // From the first column's left edge
    const GraphColumn &first = columns.constFirst();
    const GraphColumn start = { first.unixTime, first.x - first.width / 2, first.width };

    appendLinePoints(points, start);
}

void GraphPlot::appendLinePoints(GraphLinePoints &points, const GraphColumn &column) const
{
    points.in.append(linePoint(column, column.inHeight));
    points.out.append(linePoint(column, column.outHeight));
    points.total.append(linePoint(column, column.totalHeight));
}

void GraphPlot::updateScroll()
{
    // Without animation or for the empty bars, which look the same when scrolled by a second,
    // don't wake up to scroll by pixels
    if (!m_animated || m_barsEmpty) {
        m_bars->setX(0);
        return;
    }

    // Move the bars to the left smoothly: by a device pixel, up to the next second
    const int secondPixels = this->secondPixels();
    const qint64 msecs = QDateTime::currentMSecsSinceEpoch() - lastUnixTime() * 1000;
    const int shift = int(qBound(qint64(0), msecs * secondPixels / 1000, qint64(secondPixels)));

    m_bars->setX(-shift / devicePixelRatioF());

    if (shift < secondPixels) {
        // Wait for the next pixel
        const qint64 nextMsecs = ((shift + 1) * 1000 + secondPixels - 1) / secondPixels;
        m_scrollTimer.start(int(nextMsecs - msecs));
    }
}

void GraphPlot::setupSpeedArrows(const QFont &font)
{
    // By the text's height
    const qreal textHeight = QFontMetricsF(font).height();
    const QRectF rect(0, textHeight * speedArrowTop, textHeight * speedArrowWidth,
            textHeight * speedArrowHeight);

    m_speedInArrow->setPath(arrowPath(rect, /*down=*/true));
    m_speedOutArrow->setPath(arrowPath(rect, /*down=*/false));
}

void GraphPlot::updateSpeedBox()
{
    // The arrows are colored by their bars
    qreal width = layoutSpeedItem(m_speedInArrow, 0) + speedArrowSpacing;
    width = layoutSpeedItem(m_speedInText, width) + m_speedSpacing;
    width = layoutSpeedItem(m_speedOutArrow, width) + speedArrowSpacing;
    width = layoutSpeedItem(m_speedOutText, width);

    const qreal height = m_speedInText->boundingRect().height();
    const double centerX = m_axisRect.left() + m_axisRect.width() / 2.0;

    m_speedBox->setRect(QRectF(0, 0, width, height));
    m_speedBox->setPos(qRound(centerX - width / 2), m_axisRect.top());
}

void GraphPlot::updateAxes(const QVector<int> &tickYs, const QVector<int> &subTickYs)
{
    const int bottom = m_axisRect.bottom();
    const int top = bottom - m_axisRect.height();

    QPainterPath path;

    // Base Lines
    addVLine(path, m_axisRect.left(), bottom, top);

    // Right Line and Ticks
    if (m_axisTicksVisible) {
        addVLine(path, m_axisRect.right() + 1, bottom, top);

        addTicks(path, tickYs, m_axisRect, tickLength);
        addTicks(path, subTickYs, m_axisRect, subTickLength);
    }

    m_axes->setPath(path);
}

void GraphPlot::updateTickLabels(const QVector<int> &tickYs)
{
    const QRect viewRect = viewport()->rect();
    const int labelsRight = m_axisRect.left() - axisTickLength() - tickLabelPadding;
    const int count = tickYs.size();

    for (int i = 0; i < count; ++i) {
        auto label = m_tickLabels.at(i);
        const QRectF rect = label->boundingRect();
        const int y = qRound(tickYs.at(i) - rect.height() / 2);

        label->setPos(qRound(labelsRight - rect.width()), y);

        // Hide the label clipped by the view's border
        label->setVisible(y >= viewRect.top() && y + rect.height() <= viewRect.bottom());
    }
}

void GraphPlot::updateUnitLabel()
{
    const QRectF rect = m_unitLabel->boundingRect();
    const double centerY = m_axisRect.top() + m_axisRect.height() / 2.0;

    // The rotated label's text goes up from its position
    m_unitLabel->setPos(borderWidth() + axisPadding, qRound(centerY + rect.width() / 2));

    // Hide the label clipped by the view's border
    m_unitLabel->setVisible(rect.width() <= viewport()->height());
}

void GraphPlot::updateBorder()
{
    if (!m_borderVisible) {
        m_border->setRects({});
        return;
    }

    // A device pixel's lines at the view's edges
    const qreal dpr = devicePixelRatioF();
    const QSize size = viewport()->size();
    const qreal width = qRound(size.width() * dpr) / dpr;
    const qreal height = qRound(size.height() * dpr) / dpr;
    const qreal line = 1 / dpr;

    m_border->setRects({
            QRectF(0, 0, width, line),
            QRectF(0, height - line, width, line),
            QRectF(0, 0, line, height),
            QRectF(width - line, 0, line, height),
    });
}

int GraphPlot::valueToPixel(double value) const
{
    // Values above the range are clipped anyway
    const double ratio = qMin(value / m_valueUpper, 1.0);

    return qRound(m_axisRect.bottom() - ratio * m_axisRect.height());
}

QVector<int> GraphPlot::valuesToPixels(const QVector<double> &values) const
{
    QVector<int> pixels;
    pixels.reserve(values.size());

    for (const double value : values) {
        pixels.append(valueToPixel(value));
    }

    return pixels;
}

int GraphPlot::barsBottom() const
{
    // In device pixels: up to the view's bottom or to the border's line of a device pixel
    return qRound(viewport()->height() * devicePixelRatioF()) - borderWidth();
}

int GraphPlot::barHeight(quint64 bits) const
{
    if (bits == 0)
        return 0;

    // Values above the range are clipped anyway
    const double ratio = qMin(bits / m_valueUpper, 1.0);
    const int height = qRound(ratio * m_axisRect.height() * devicePixelRatioF());

    // At least a pixel for the small traffic
    return qMax(height, 1);
}

QVector<GraphColumn> GraphPlot::visibleColumns(qint64 keyLower) const
{
    const qreal dpr = devicePixelRatioF();
    const int secondPixels = this->secondPixels();
    const qreal barWidth = secondPixels / dpr; // without gaps

    // The last second's middle is at the right edge, in device pixels
    const int right = qFloor((m_axisRect.left() + m_axisRect.width()) * dpr) - secondPixels / 2;
    const qint64 keyUpper = lastUnixTime();

    QVector<GraphColumn> columns;

    for (const auto &point : std::as_const(m_points)) {
        if (point.unixTime < keyLower)
            continue;

        const qreal x = (right - int(keyUpper - point.unixTime) * secondPixels) / dpr;

        columns.append(columnAt(x, barWidth, point));
    }

    return columns;
}

GraphColumn GraphPlot::columnAt(qreal x, qreal width, const GraphPoint &point) const
{
    const GraphBits bits = visibleBits(point);

    return { point.unixTime, x, width, barHeight(bits.in), barHeight(bits.out),
        barHeight(bits.total) };
}

GraphColumnRects GraphPlot::columnRects(const GraphColumn &column) const
{
    // The smaller bar is in front of the bigger one: the bigger one is visible above it only
    const int frontHeight = qMin(column.inHeight, column.outHeight);
    const int backHeight = qMax(column.inHeight, column.outHeight);

    const QRectF frontRect = barRect(column, 0, frontHeight);
    const QRectF backRect = barRect(column, frontHeight, backHeight);

    // The total (in + out) is above the bigger one
    const int totalHeight = qMax(column.totalHeight, backHeight);
    const QRectF totalRect = barRect(column, backHeight, totalHeight);

    if (column.inHeight <= column.outHeight) {
        return { frontRect, backRect, totalRect };
    }

    return { backRect, frontRect, totalRect };
}

QRectF GraphPlot::barRect(const GraphColumn &column, int fromHeight, int toHeight) const
{
    // In device pixels: the adjacent bars have no gaps with a fractional scale
    const qreal dpr = devicePixelRatioF();
    const int bottom = barsBottom();

    return QRectF(column.x, (bottom - toHeight) / dpr, column.width, (toHeight - fromHeight) / dpr);
}

QPointF GraphPlot::linePoint(const GraphColumn &column, int height) const
{
    const qreal y = (barsBottom() - lineCenterOffset - height) / devicePixelRatioF();

    return { column.x + column.width / 2, y };
}
