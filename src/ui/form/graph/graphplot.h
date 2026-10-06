#ifndef GRAPHPLOT_H
#define GRAPHPLOT_H

#include <QGraphicsView>
#include <QTimer>
#include <QVarLengthArray>

#include "axistickerspeed.h"
#include "graphanimation.h"

class GraphBarsItem;

class QGraphicsPathItem;
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;

struct GraphPoint
{
    qint64 unixTime = 0;
    quint64 inBits = 0;
    quint64 outBits = 0;
};

// A second's bars: in and out are overlapped
struct GraphColumn
{
    qreal x = 0;
    qreal width = 0;
    int inHeight = 0; // device pixels
    int outHeight = 0; // device pixels
    int totalHeight = 0; // device pixels: of in + out
};

struct GraphColumnRects
{
    QRectF in;
    QRectF out;
    QRectF total;
};

struct GraphBarsRects
{
    QVector<QRectF> in;
    QVector<QRectF> out;
    QVector<QRectF> total;
};

class GraphPlot : public QGraphicsView
{
    Q_OBJECT

public:
    enum ColorType : qint8 {
        ColorBg = 0,
        ColorBorder,
        ColorIn,
        ColorOut,
        ColorTotal,
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

    bool animated() const { return m_animated; }
    void setAnimated(bool v);

    bool axisTicksVisible() const { return m_axisTicksVisible; }
    void setAxisTicksVisible(bool v);

    bool borderVisible() const { return m_borderVisible; }
    void setBorderVisible(bool v);

    FormatUtil::SizeFormat unitFormat() const { return m_ticker.unitFormat(); }
    void setUnitFormat(FormatUtil::SizeFormat v);

    bool speedVisible() const;
    void setSpeedVisible(bool v);

    void setSpeedText(const QString &inText, const QString &outText);

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
    double targetValueUpper(quint64 bits) const;
    void animateValueUpper(double v);
    void setValueUpper(double v);
    void updateScale(double value);

    void redraw();

    void layoutAxes();

    void setupTickLabelStyle(QGraphicsSimpleTextItem *label) const;
    void updateTickLabelsStyle();

    int setupTickLabels(const QStringList &labels);
    void updateAxisRect(int tickLabelsWidth);
    int axisTickLength() const;
    int borderWidth() const;
    int unitLabelWidth() const;

    void updateGrid(const QVector<int> &tickYs);
    void startRising(qint64 unixTime);
    void updateBars();
    void updateBarsOutline();
    void updateRisingBars();
    void updateScroll();
    void setupSpeedArrows(const QFont &font);
    void updateSpeedBox();
    void updateAxes(const QVector<int> &tickYs, const QVector<int> &subTickYs);
    void updateTickLabels(const QVector<int> &tickYs);
    void updateUnitLabel();
    void updateBorder();

    int valueToPixel(double value) const;
    QVector<int> valuesToPixels(const QVector<double> &values) const;
    int barsBottom() const;
    int barHeight(quint64 bits) const;
    GraphColumn columnAt(qreal x, qreal width, const GraphPoint &point) const;
    GraphColumnRects columnRects(const GraphColumn &column) const;
    QRectF barRect(const GraphColumn &column, int fromHeight, int toHeight) const;

private:
    bool m_mousePressed : 1 = false;
    bool m_mouseDragging : 1 = false;
    bool m_mouseHasMoved : 1 = false;
    bool m_animated : 1 = true;
    bool m_axisTicksVisible : 1 = false;
    bool m_borderVisible : 1 = false;
    bool m_axesChanged : 1 = true;
    bool m_barsEmpty : 1 = true;

    int m_maxSeconds = 500;

    double m_valueUpper = 5; // shown
    double m_valueTarget = 5;

    qint64 m_fixedValueMax = 0;
    qint64 m_risingTime = 0;

    QGraphicsPathItem *m_grid = nullptr;
    QGraphicsRectItem *m_plotArea = nullptr;
    QGraphicsRectItem *m_bars = nullptr;
    GraphBarsItem *m_barsIn = nullptr;
    GraphBarsItem *m_barsOut = nullptr;
    GraphBarsItem *m_risingIn = nullptr;
    GraphBarsItem *m_risingOut = nullptr;
    GraphBarsItem *m_barsTotal = nullptr;
    GraphBarsItem *m_risingTotal = nullptr;
    QGraphicsRectItem *m_speedBox = nullptr;
    QGraphicsPathItem *m_speedInArrow = nullptr;
    QGraphicsSimpleTextItem *m_speedInText = nullptr;
    QGraphicsPathItem *m_speedOutArrow = nullptr;
    QGraphicsSimpleTextItem *m_speedOutText = nullptr;
    QGraphicsPathItem *m_axes = nullptr;
    QGraphicsSimpleTextItem *m_unitLabel = nullptr;
    GraphBarsItem *m_border = nullptr;

    QPoint m_mousePressPos;
    QRect m_axisRect;

    GraphColumn m_risingColumn;
    GraphBarsRects m_risingNeighbors;

    QColor m_tickLabelColor;
    QFont m_tickLabelFont;

    AxisTickerSpeed m_ticker;

    QTimer m_scrollTimer;
    GraphAnimation m_risingAnimation;
    GraphAnimation m_scaleAnimation;

    QList<QGraphicsSimpleTextItem *> m_tickLabels;
    QList<GraphPoint> m_points;
};

#endif // GRAPHPLOT_H
