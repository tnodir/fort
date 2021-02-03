#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include "../../util/window/widgetwindow.h"

class FortSettings;
class GraphPlot;
class QCPBars;

class GraphWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(FortSettings *fortSettings, QWidget *parent = nullptr);

signals:
    void mouseRightClick(QMouseEvent *event);

public slots:
    void updateWindowFlags();
    void updateColors();

    void addTraffic(qint64 unixTime, quint32 inBytes, quint32 outBytes);

private slots:
    void checkHoverLeave();

    void addEmptyTraffic();

private:
    void onMouseDoubleClick(QMouseEvent *event);
    void onMouseDragBegin(QMouseEvent *event);
    void onMouseDragMove(QMouseEvent *event);
    void onMouseDragEnd(QMouseEvent *event);

    void cancelMousePressAndDragging();

private:
    void setupUi();
    void setupTimer();

    void addData(QCPBars *graph, qint64 rangeLower, qint64 unixTime, quint32 bytes);

    void updateWindowTitleSpeed();
    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

    static QPen adjustPen(const QPen &pen, const QColor &color);

protected:
    void enterEvent(
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QEvent *event
#else
            QEnterEvent *event
#endif
            ) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_mouseDragResize = false;

    qint64 m_lastUnixTime = 0;

    FortSettings *m_fortSettings = nullptr;

    GraphPlot *m_plot = nullptr;
    QCPBars *m_graphIn = nullptr;
    QCPBars *m_graphOut = nullptr;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QTimer m_updateTimer;
    QTimer m_hoverTimer;
};

#endif // GRAPHWINDOW_H
