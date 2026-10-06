#ifndef GRAPHPLOT_H
#define GRAPHPLOT_H

#include <QGraphicsView>
#include <QTimer>
#include <QVarLengthArray>
#include <QVariantAnimation>

#include "axistickerspeed.h"

class QGraphicsPathItem;
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;

struct GraphPoint
{
    qint64 unixTime = 0;
    quint64 inBits = 0;
    quint64 outBits = 0;
};

class GraphPlot : public QGraphicsView
{
    Q_OBJECT

public:
    enum ColorType : qint8 {
        ColorBg = 0,
        ColorIn,
        ColorOut,
        ColorAxis,
        ColorTickLabel,
        ColorLabel,
        ColorGrid,
        ColorCount
    };

    using ColorArray = QVarLengthArray<QColor, ColorCount>;

    explicit GraphPlot(QWidget *parent = nullptr);

    bool mousePressed() const { return m_mousePressed; }
    bool mouseDragging() const { return m_mouseDragging; }

    int maxSeconds() const { return m_maxSeconds; }
    void setMaxSeconds(int v) { m_maxSeconds = v; }

    qint64 fixedValueMax() const { return m_fixedValueMax; }
    void setFixedValueMax(qint64 v) { m_fixedValueMax = v; }

    FormatUtil::SizeFormat unitFormat() const { return m_ticker.unitFormat(); }
    void setUnitFormat(FormatUtil::SizeFormat v);

    bool speedVisible() const;
    void setSpeedVisible(bool v);

    void setSpeedText(const QString &text);

    GraphPoint pointAt(qint64 unixTime) const;

    void setColors(const GraphPlot::ColorArray &colors);
    void setTickLabelSize(int pointSize);

    void addPoint(const GraphPoint &point);

    void cancelMousePressAndDragging();

signals:
    void resized(QResizeEvent *event);
    void mouseDoubleClick(QMouseEvent *event);
    void mouseRightClick(QMouseEvent *event);
    void mouseDragBegin(QMouseEvent *event);
    void mouseDragMove(QMouseEvent *event);
    void mouseDragEnd(QMouseEvent *event);

public slots:
    void replot();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool checkMouseHasMoved(QMouseEvent *event);

    void setupView();
    void setupItems();
    void setupScrollTimer();
    void setupRisingAnimation();
    void setupScaleAnimation();

    int pointIndex(qint64 unixTime) const;
    bool isPointAt(int index, qint64 unixTime) const;

    void removeOldPoints(qint64 unixTime);
    void mergePoint(const GraphPoint &point);

    qint64 lastUnixTime() const;
    int secondPixels() const;
    int keyRangeSize() const;
    qint64 keyLower() const;

    quint64 maxBits(qint64 keyLower) const;
    double targetValueUpper(qint64 keyLower) const;
    void animateValueUpper(double v);
    void setValueUpper(double v);
    void updateScale(const QVariant &value);

    void redraw();

    void layoutAxes();

    void setupTickLabelStyle(QGraphicsSimpleTextItem *label) const;
    void updateTickLabelsStyle();

    int setupTickLabels(const QStringList &labels);
    void updateAxisRect(int tickLabelsWidth);

    void updateGrid(const QVector<int> &tickYs);
    void startRising(qint64 unixTime);
    void updateBars();
    void updateRisingBars();
    void updateScroll();
    void updateSpeedBox();
    void updateAxes(const QVector<int> &tickYs, const QVector<int> &subTickYs);
    void updateTickLabels(const QVector<int> &tickYs);

    int valueToPixel(double value) const;
    QVector<int> valuesToPixels(const QVector<double> &values) const;
    QRectF barRect(qreal x, qreal width, quint64 bits) const;

private:
    bool m_mousePressed : 1 = false;
    bool m_mouseDragging : 1 = false;
    bool m_mouseHasMoved : 1 = false;
    bool m_axesChanged : 1 = true;

    int m_maxSeconds = 500;

    double m_valueUpper = 5; // shown
    double m_valueTarget = 5;

    qint64 m_fixedValueMax = 0;
    qint64 m_risingTime = 0;

    QGraphicsPathItem *m_grid = nullptr;
    QGraphicsRectItem *m_plotArea = nullptr;
    QGraphicsRectItem *m_bars = nullptr;
    QGraphicsPathItem *m_barsIn = nullptr;
    QGraphicsPathItem *m_barsOut = nullptr;
    QGraphicsRectItem *m_risingIn = nullptr;
    QGraphicsRectItem *m_risingOut = nullptr;
    QGraphicsRectItem *m_speedBox = nullptr;
    QGraphicsSimpleTextItem *m_speedText = nullptr;
    QGraphicsPathItem *m_axes = nullptr;

    QPoint m_mousePressPos;
    QRect m_axisRect;

    QRectF m_risingInRect;
    QRectF m_risingOutRect;

    QColor m_tickLabelColor;
    QFont m_tickLabelFont;

    AxisTickerSpeed m_ticker;

    QTimer m_scrollTimer;
    QVariantAnimation m_risingAnimation;
    QVariantAnimation m_scaleAnimation;

    QList<QGraphicsSimpleTextItem *> m_tickLabels;
    QList<GraphPoint> m_points;
};

#endif // GRAPHPLOT_H
