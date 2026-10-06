#include "graphplot.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QtMath>

namespace {

inline constexpr int axisPadding = 2;
inline constexpr int axisMarginTop = 2;
inline constexpr int axisMarginBottom = 1;
inline constexpr int tickLength = 5;
inline constexpr int subTickLength = 2;
inline constexpr int tickLabelPadding = 2;
inline constexpr int keyPixels = 4; // pixels per second
inline constexpr int barWidth = 2;
inline constexpr int speedBgAlpha = 130;
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

void addTicks(QPainterPath &path, const QVector<int> &ys, const QRect &axisRect, int length)
{
    const int left = axisRect.left();
    const int right = axisRect.right() + 1;

    for (const int y : ys) {
        addHLine(path, left - length, left, y);
        addHLine(path, right, right + length, y);
    }
}

QGraphicsPathItem *createBars(QGraphicsItem *parent)
{
    auto bars = new QGraphicsPathItem(parent);
    bars->setPen(Qt::NoPen);

    return bars;
}

}

GraphPlot::GraphPlot(QWidget *parent) : QGraphicsView(parent)
{
    setupView();
    setupItems();
}

bool GraphPlot::speedVisible() const
{
    return m_speedBox->isVisible();
}

void GraphPlot::setSpeedVisible(bool v)
{
    m_speedBox->setVisible(v);
}

void GraphPlot::setSpeedText(const QString &text)
{
    m_speedText->setText(text);
}

GraphPoint GraphPlot::pointAt(qint64 unixTime) const
{
    for (auto it = m_points.crbegin(); it != m_points.crend(); ++it) {
        if (it->unixTime <= unixTime) {
            return (it->unixTime == unixTime) ? *it : GraphPoint();
        }
    }

    return {};
}

void GraphPlot::setColors(const ColorArray &colors)
{
    // Background Color
    const QColor bgColor = colors[ColorBg];
    const bool isTransparentBg = (bgColor == Qt::transparent);

    setBackgroundBrush(isTransparentBg ? QBrush(Qt::NoBrush) : QBrush(bgColor));

    // Grid
    m_grid->setPen(QPen(colors[ColorGrid], 0, Qt::DotLine));

    // Graph Inbound
    m_barsIn->setBrush(colors[ColorIn]);

    // Graph Outbound
    m_barsOut->setBrush(colors[ColorOut]);

    // Text Speed
    {
        QColor speedBgColor = bgColor;
        speedBgColor.setAlpha(speedBgAlpha);

        m_speedBox->setBrush(speedBgColor);
        m_speedText->setBrush(colors[ColorLabel]);
    }

    // Axis
    m_axes->setPen(QPen(colors[ColorAxis], 0, Qt::SolidLine, Qt::SquareCap));

    m_tickLabelColor = colors[ColorTickLabel];

    updateTickLabelsStyle();
}

void GraphPlot::setTickLabelSize(int pointSize)
{
    m_tickLabelFont = font();
    m_tickLabelFont.setPointSize(pointSize);

    // Text Speed
    QFont speedFont = m_tickLabelFont;
    speedFont.setPointSize(pointSize + 1);
    speedFont.setWeight(QFont::DemiBold);

    m_speedText->setFont(speedFont);

    updateTickLabelsStyle();
}

void GraphPlot::addPoint(GraphPoint point, qint64 rangeLower)
{
    removeOldPoints(point.unixTime, rangeLower);
    mergeLastPoint(point);

    m_points.append(point);
}

void GraphPlot::cancelMousePressAndDragging()
{
    m_mousePressed = false;
    m_mouseDragging = false;
}

void GraphPlot::replot()
{
    const int keyRangeSize = this->keyRangeSize();
    const qint64 keyLower = lastUnixTime() - keyRangeSize;

    updateValueRange(keyLower);

    AxisTicks axisTicks;
    m_ticker.generate(m_valueUpper, axisTicks);

    const int tickLabelsWidth = setupTickLabels(axisTicks.labels);
    updateAxisRect(tickLabelsWidth);

    const QVector<int> tickYs = valuesToPixels(axisTicks.ticks);
    const QVector<int> subTickYs = valuesToPixels(axisTicks.subTicks);

    updateGrid(tickYs);
    updateBars(keyLower, keyRangeSize);
    updateSpeedBox();
    updateAxes(tickYs, subTickYs);
    updateTickLabels(tickYs);
}

void GraphPlot::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    setSceneRect(viewport()->rect());

    emit resized(event);
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
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

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

    // Graph Inbound
    m_barsIn = createBars(m_plotArea);

    // Graph Outbound
    m_barsOut = createBars(m_plotArea);

    // Text Speed
    m_speedBox = new QGraphicsRectItem(m_plotArea);
    m_speedBox->setPen(Qt::NoPen);

    m_speedText = new QGraphicsSimpleTextItem(m_speedBox);

    // Axis
    m_axes = scene->addPath(QPainterPath());

    m_tickLabelFont = font();
}

void GraphPlot::removeOldPoints(qint64 unixTime, qint64 rangeLower)
{
    if (m_points.isEmpty())
        return;

    const qint64 lastTime = m_points.constLast().unixTime;
    if (rangeLower > lastTime || unixTime < lastTime) {
        m_points.clear();
        return;
    }

    while (m_points.constFirst().unixTime < rangeLower) {
        m_points.removeFirst();
    }
}

void GraphPlot::mergeLastPoint(GraphPoint &point)
{
    if (m_points.isEmpty() || m_points.constLast().unixTime != point.unixTime)
        return;

    const GraphPoint lastPoint = m_points.takeLast();

    point.inBits += lastPoint.inBits;
    point.outBits += lastPoint.outBits;
}

qint64 GraphPlot::lastUnixTime() const
{
    return m_points.isEmpty() ? 0 : m_points.constLast().unixTime;
}

int GraphPlot::keyRangeSize() const
{
    const int width = m_axisRect.isNull() ? viewport()->width() : m_axisRect.width();

    return qMax(width / keyPixels, 1);
}

quint64 GraphPlot::maxBits(qint64 keyLower) const
{
    quint64 bits = 0;

    for (const auto &point : m_points) {
        if (point.unixTime >= keyLower) {
            bits = qMax(bits, qMax(point.inBits, point.outBits));
        }
    }

    return bits;
}

void GraphPlot::updateValueRange(qint64 keyLower)
{
    if (m_fixedValueMax > 0) {
        m_valueUpper = double(m_fixedValueMax);
        return;
    }

    // Keep the current range for empty traffic
    const quint64 bits = maxBits(keyLower);
    if (bits > 0) {
        m_valueUpper = double(bits);
    }
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
    const int leftMargin = axisPadding + tickLength + tickLabelPadding + tickLabelsWidth;
    const int rightMargin = tickLength;

    m_axisRect =
            viewport()->rect().adjusted(leftMargin, axisMarginTop, -rightMargin, -axisMarginBottom);

    m_plotArea->setRect(m_axisRect);
}

void GraphPlot::updateGrid(const QVector<int> &tickYs)
{
    QPainterPath path;

    for (const int y : tickYs) {
        addHLine(path, m_axisRect.left(), m_axisRect.right(), y);
    }

    m_grid->setPath(path);
}

void GraphPlot::updateBars(qint64 keyLower, int keyRangeSize)
{
    const int left = m_axisRect.left();
    const double keyScale = double(m_axisRect.width()) / keyRangeSize;

    QPainterPath pathIn;
    QPainterPath pathOut;

    for (const auto &point : std::as_const(m_points)) {
        if (point.unixTime < keyLower)
            continue;

        const int x = left + qRound((point.unixTime - keyLower) * keyScale);

        pathIn.addRect(barRect(x - barWidth, point.inBits));
        pathOut.addRect(barRect(x, point.outBits));
    }

    // Overlapping bars must not make holes
    pathIn.setFillRule(Qt::WindingFill);
    pathOut.setFillRule(Qt::WindingFill);

    m_barsIn->setPath(pathIn);
    m_barsOut->setPath(pathOut);
}

void GraphPlot::updateSpeedBox()
{
    const QRectF textRect = m_speedText->boundingRect();
    const double centerX = m_axisRect.left() + m_axisRect.width() / 2.0;

    m_speedBox->setRect(QRectF(QPointF(0, 0), textRect.size()));
    m_speedBox->setPos(qRound(centerX - textRect.width() / 2), m_axisRect.top());
}

void GraphPlot::updateAxes(const QVector<int> &tickYs, const QVector<int> &subTickYs)
{
    const int bottom = m_axisRect.bottom();
    const int top = bottom - m_axisRect.height();

    QPainterPath path;

    // Base Lines
    addVLine(path, m_axisRect.left(), bottom, top);
    addVLine(path, m_axisRect.right() + 1, bottom, top);

    // Ticks
    addTicks(path, tickYs, m_axisRect, tickLength);
    addTicks(path, subTickYs, m_axisRect, subTickLength);

    m_axes->setPath(path);
}

void GraphPlot::updateTickLabels(const QVector<int> &tickYs)
{
    const QRect viewRect = viewport()->rect();
    const int labelsRight = m_axisRect.left() - tickLength - tickLabelPadding;
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

QRectF GraphPlot::barRect(int x, quint64 bits) const
{
    const int bottom = m_axisRect.bottom();
    const int y = qMin(valueToPixel(double(bits)), bottom);

    return QRectF(x, y, barWidth, bottom - y + 1);
}
