#ifndef GRAPHPLOT_H
#define GRAPHPLOT_H

#include "qcustomplot.h"

class GraphPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit GraphPlot(QWidget *parent = 0);

signals:
    void mouseDragBegin(QMouseEvent *event);
    void mouseDragMove(QMouseEvent *event);
    void mouseDragEnd(QMouseEvent *event);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    uint m_mousePressed     : 1;
    uint m_mouseDragging    : 1;
};

#endif // GRAPHPLOT_H
