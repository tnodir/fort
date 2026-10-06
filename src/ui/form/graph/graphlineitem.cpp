#include "graphlineitem.h"

#include <QPainter>
#include <QtMath>

namespace {

inline constexpr qreal fillTopOpacity = 0.4; // of the line's color
inline constexpr qreal fillBottomOpacity = 0.1;

QColor opacityColor(const QColor &color, qreal opacity)
{
    QColor c = color;
    c.setAlphaF(color.alphaF() * opacity);

    return c;
}

// Fritsch-Carlson: limit the tangents of a segment to keep the curve monotonic
void limitTangents(qreal secant, qreal &tangent0, qreal &tangent1)
{
    if (secant == 0) {
        tangent0 = tangent1 = 0;
        return;
    }

    const qreal a = tangent0 / secant;
    const qreal b = tangent1 / secant;
    const qreal sum = a * a + b * b;

    if (sum <= 9)
        return;

    const qreal k = 3 / qSqrt(sum);

    tangent0 = k * a * secant;
    tangent1 = k * b * secant;
}

QVector<qreal> secantsOf(const QVector<QPointF> &points)
{
    const int count = int(points.size());

    QVector<qreal> secants(count - 1);

    for (int i = 0; i < count - 1; ++i) {
        const QPointF d = points.at(i + 1) - points.at(i);

        secants[i] = (d.x() != 0) ? d.y() / d.x() : 0;
    }

    return secants;
}

QVector<qreal> tangentsOf(const QVector<qreal> &secants)
{
    const int count = int(secants.size()) + 1;

    QVector<qreal> tangents(count);
    tangents[0] = secants.constFirst();
    tangents[count - 1] = secants.constLast();

    // Flat at the local extremes
    for (int i = 1; i < count - 1; ++i) {
        const qreal s0 = secants.at(i - 1);
        const qreal s1 = secants.at(i);

        tangents[i] = (s0 * s1 > 0) ? (s0 + s1) / 2 : 0;
    }

    for (int i = 0; i < count - 1; ++i) {
        limitTangents(secants.at(i), tangents[i], tangents[i + 1]);
    }

    return tangents;
}

QPainterPath smoothPath(const QVector<QPointF> &points)
{
    QPainterPath path;

    if (points.size() < 2)
        return path;

    const QVector<qreal> tangents = tangentsOf(secantsOf(points));
    const int count = int(points.size());

    path.moveTo(points.constFirst());

    // Cubic Hermite segments as Bezier curves
    for (int i = 0; i < count - 1; ++i) {
        const QPointF &p0 = points.at(i);
        const QPointF &p1 = points.at(i + 1);
        const qreal dx = (p1.x() - p0.x()) / 3;

        path.cubicTo(p0 + QPointF(dx, tangents.at(i) * dx),
                p1 - QPointF(dx, tangents.at(i + 1) * dx), p1);
    }

    return path;
}

}

GraphLineItem::GraphLineItem(QGraphicsItem *parent) : GraphCachedItem(parent)
{
    // The scrolled line isn't repainted
    setCached(true);
}

void GraphLineItem::setColor(const QColor &v)
{
    if (m_color == v)
        return;

    m_color = v;

    updateFill();
}

void GraphLineItem::setFillVisible(bool v)
{
    if (m_fillVisible == v)
        return;

    m_fillVisible = v;

    updateFill();
}

void GraphLineItem::setPoints(const QVector<QPointF> &v)
{
    if (m_points == v)
        return;

    m_points = v;
    m_path = smoothPath(v);

    updateFill();
}

void GraphLineItem::updateFill()
{
    prepareGeometryChange();

    m_fillPath.clear();
    m_fillBrush = QBrush();

    if (m_fillVisible && m_points.size() >= 2) {
        setupFill();
    }

    // With the offset lines and the antialiasing
    const QRectF rect = m_path.controlPointRect() | m_fillPath.controlPointRect();
    m_boundingRect = rect.isNull() ? QRectF() : rect.adjusted(-1, -1, 2, 2);

    invalidateCache();
}

void GraphLineItem::setupFill()
{
    // Down to the zero: the lowest point
    qreal bottom = 0;
    for (const QPointF &point : std::as_const(m_points)) {
        bottom = qMax(bottom, point.y());
    }
    bottom += 2; // below the line, clipped by the plot area

    m_fillPath = m_path;
    m_fillPath.lineTo(m_points.constLast().x(), bottom);
    m_fillPath.lineTo(m_points.constFirst().x(), bottom);
    m_fillPath.closeSubpath();

    // Fading to the bottom
    const QRectF fillRect = m_fillPath.controlPointRect();

    QLinearGradient gradient(0, fillRect.top(), 0, fillRect.bottom());
    gradient.setColorAt(0, opacityColor(m_color, fillTopOpacity));
    gradient.setColorAt(1, opacityColor(m_color, fillBottomOpacity));

    m_fillBrush = QBrush(gradient);
}

void GraphLineItem::paintItem(QPainter *painter)
{
    if (m_points.size() < 2)
        return;

    paintFill(painter);
    paintLine(painter);
}

void GraphLineItem::paintFill(QPainter *painter)
{
    if (!m_fillVisible)
        return;

    // Aliased: its top edge is below the line
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_fillBrush);
    painter->drawPath(m_fillPath);
}

void GraphLineItem::paintLine(QPainter *painter)
{
    // The cosmetic lines of a device pixel are ~10 times faster than a thick line:
    // draw them with offsets by a device pixel
    const qreal pixel = 1 / painter->device()->devicePixelRatioF();
    const bool antialiased = painter->testRenderHint(QPainter::Antialiasing);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(m_color, 0));
    painter->setBrush(Qt::NoBrush);

    for (const QPointF offset :
            { QPointF(0, 0), QPointF(pixel, 0), QPointF(0, pixel), QPointF(pixel, pixel) }) {
        painter->translate(offset);
        painter->drawPath(m_path);
        painter->translate(-offset);
    }

    painter->setRenderHint(QPainter::Antialiasing, antialiased);
}
