#include "graphspeeditem.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPen>

namespace {

inline constexpr int speedBgAlpha = 130;
inline constexpr int speedArrowSpacing = 2;
inline constexpr char speedSampleText[] = "888.88 MiB/s"; // the widest speed text
inline constexpr double speedArrowTop = 0.18; // of the text's height
inline constexpr double speedArrowWidth = 0.55; // of the text's height
inline constexpr double speedArrowHeight = 0.66; // of the text's height
inline constexpr double speedArrowShaft = 0.36; // of the arrow's width
inline constexpr double speedArrowHead = 0.45; // of the arrow's height

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

void layoutSpeedRow(QGraphicsItem *arrow, QGraphicsItem *text, qreal y, qreal width)
{
    arrow->setPos(0, y);
    text->setPos(width - text->boundingRect().right(), y);
}

// Smooth, unlike the other items aligned to the pixels
class SmoothPathItem : public QGraphicsPathItem
{
public:
    explicit SmoothPathItem(QGraphicsItem *parent) : QGraphicsPathItem(parent)
    {
        setPen(Qt::NoPen);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget = nullptr) override
    {
        const bool antialiased = painter->testRenderHint(QPainter::Antialiasing);

        painter->setRenderHint(QPainter::Antialiasing);
        QGraphicsPathItem::paint(painter, option, widget);
        painter->setRenderHint(QPainter::Antialiasing, antialiased);
    }
};

}

GraphSpeedItem::GraphSpeedItem(QGraphicsItem *parent) :
    QGraphicsRectItem(parent),
    m_inArrow(new SmoothPathItem(this)),
    m_inText(new QGraphicsSimpleTextItem(this)),
    m_outArrow(new SmoothPathItem(this)),
    m_outText(new QGraphicsSimpleTextItem(this))
{
    setPen(Qt::NoPen);
}

void GraphSpeedItem::setText(const QString &inText, const QString &outText)
{
    m_inText->setText(inText);
    m_outText->setText(outText);
}

void GraphSpeedItem::setColors(const QColor &bgColor, const QColor &inColor, const QColor &outColor,
        const QColor &textColor)
{
    m_bgColor = bgColor;
    m_bgColor.setAlpha(speedBgAlpha);

    updateBackground();

    m_inArrow->setBrush(inColor);
    m_inText->setBrush(textColor);
    m_outArrow->setBrush(outColor);
    m_outText->setBrush(textColor);
}

void GraphSpeedItem::setBackgroundVisible(bool v)
{
    if (m_backgroundVisible == v)
        return;

    m_backgroundVisible = v;

    updateBackground();
}

void GraphSpeedItem::setTextFont(const QFont &font)
{
    m_inText->setFont(font);
    m_outText->setFont(font);

    m_spacing = QFontMetricsF(font).horizontalAdvance(QLatin1String("  "));

    setupArrows(font);
}

qreal GraphSpeedItem::rowsWidth() const
{
    const QFontMetricsF metrics(m_inText->font());

    return m_inArrow->boundingRect().right() + speedArrowSpacing
            + metrics.horizontalAdvance(QLatin1String(speedSampleText));
}

qreal GraphSpeedItem::textHeight() const
{
    return QFontMetricsF(m_inText->font()).height();
}

QSizeF GraphSpeedItem::layout(bool rows)
{
    const QSizeF size = rows ? layoutRows() : layoutLine();

    setRect(QRectF(QPointF(), size));

    return size;
}

void GraphSpeedItem::updateBackground()
{
    setBrush(m_backgroundVisible ? QBrush(m_bgColor) : QBrush(Qt::NoBrush));
}

void GraphSpeedItem::setupArrows(const QFont &font)
{
    // By the text's height
    const qreal textHeight = QFontMetricsF(font).height();
    const QRectF rect(0, textHeight * speedArrowTop, textHeight * speedArrowWidth,
            textHeight * speedArrowHeight);

    m_inArrow->setPath(arrowPath(rect, /*down=*/true));
    m_outArrow->setPath(arrowPath(rect, /*down=*/false));
}

QSizeF GraphSpeedItem::layoutLine()
{
    qreal width = layoutSpeedItem(m_inArrow, 0) + speedArrowSpacing;
    width = layoutSpeedItem(m_inText, width) + m_spacing;
    width = layoutSpeedItem(m_outArrow, width) + speedArrowSpacing;
    width = layoutSpeedItem(m_outText, width);

    return { width, textHeight() };
}

QSizeF GraphSpeedItem::layoutRows()
{
    // The in and out rows: the arrows at the left, the texts at the right
    const qreal width = rowsWidth();
    const qreal height = textHeight();

    layoutSpeedRow(m_inArrow, m_inText, 0, width);
    layoutSpeedRow(m_outArrow, m_outText, height, width);

    return { width, height * 2 };
}
