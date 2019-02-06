#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include "../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(GraphPlot)
QT_FORWARD_DECLARE_CLASS(QCPBars)

class GraphWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(FortSettings *fortSettings,
                         QWidget *parent = nullptr);

signals:

public slots:
    void addTraffic(qint64 unixTime, qint32 inBytes, qint32 outBytes);

private slots:
    void setupWindow();

    void addEmptyTraffic();

private:
    void onMouseDoubleClick(QMouseEvent *event);
    void onMouseDragBegin(QMouseEvent *event);
    void onMouseDragMove(QMouseEvent *event);
    void onMouseDragEnd(QMouseEvent *event);

private:
    void setupUi();
    void setupTimer();

    void addData(QCPBars *graph, qint64 rangeLower,
                 qint64 unixTime, qint32 bytes);

private:
    FortSettings *m_fortSettings;

    GraphPlot *m_plot;
    QCPBars *m_graphIn;
    QCPBars *m_graphOut;

    QPoint m_mousePressOffset;

    QTimer m_timer;
};

#endif // GRAPHWINDOW_H
