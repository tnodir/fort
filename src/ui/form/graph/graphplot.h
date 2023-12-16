#ifndef GRAPHPLOT_H
#define GRAPHPLOT_H

#include <qcustomplot.h>

class GraphPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit GraphPlot(QWidget *parent = nullptr);

    bool mousePressed() const { return m_mousePressed; }
    bool mouseDragging() const { return m_mouseDragging; }

    void cancelMousePressAndDragging();

signals:
    void resized(QResizeEvent *event);
    void mouseRightClick(QMouseEvent *event);
    void mouseDragBegin(QMouseEvent *event);
    void mouseDragMove(QMouseEvent *event);
    void mouseDragEnd(QMouseEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_mousePressed : 1 = false;
    bool m_mouseDragging : 1 = false;
};

#endif // GRAPHPLOT_H
