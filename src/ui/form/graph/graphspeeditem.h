#ifndef GRAPHSPEEDITEM_H
#define GRAPHSPEEDITEM_H

#include <QGraphicsRectItem>

#include <utility>

class QGraphicsPathItem;
class QGraphicsSimpleTextItem;

// The text speed: the in and out texts after their arrows, in a line or in two rows,
// the out one first if swapped
class GraphSpeedItem : public QGraphicsRectItem
{
public:
    explicit GraphSpeedItem(QGraphicsItem *parent = nullptr);

    void setText(const QString &inText, const QString &outText);

    // The arrows are colored by their bars
    void setColors(const QColor &bgColor, const QColor &inColor, const QColor &outColor,
            const QColor &textColor);

    // The background covers the graph under the texts
    void setBackgroundVisible(bool v);

    void setTextFont(const QFont &font);

    void setSwapped(bool v) { m_swapped = v; }

    // By the widest speed text: the graph doesn't move with the speed
    qreal rowsWidth() const;

    // By the font: the empty text has no height
    qreal textHeight() const;

    // Lays out the texts and sets the box's rectangle by their size
    QSizeF layout(bool rows);

private:
    struct Part
    {
        QGraphicsItem *arrow = nullptr;
        QGraphicsItem *text = nullptr;
    };

    // In the shown order
    std::pair<Part, Part> orderedParts() const;

    void updateBackground();

    void setupArrows(const QFont &font);

    QSizeF layoutLine();
    QSizeF layoutRows();

private:
    bool m_swapped = false;
    bool m_backgroundVisible = true;

    qreal m_spacing = 0; // between the in and out texts

    QGraphicsPathItem *m_inArrow = nullptr;
    QGraphicsSimpleTextItem *m_inText = nullptr;
    QGraphicsPathItem *m_outArrow = nullptr;
    QGraphicsSimpleTextItem *m_outText = nullptr;

    QColor m_bgColor;
};

#endif // GRAPHSPEEDITEM_H
