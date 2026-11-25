#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include <form/controls/formwindow.h>
#include <form/graph/graphview.h>

class GraphWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(int graphId, QWidget *parent = nullptr);

    int graphId() const { return iniGraph().graphId(); }

    WindowCode windowCode() const override { return WindowGraph; }
    QString helpSection() const override { return "traffic-graph"; }
    QString iniGroup() const override;
    bool deleteOnClose() const override;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    void saveWindowGeometry();

private slots:
    void checkHoverLeave();

    void updateWindowGeometry();

    void updateWindowTitle(const QString &inText, const QString &outText);

private:
    const IniGraph &iniGraph() const { return m_view->iniGraph(); }
    IniGraph &iniGraph() { return m_view->iniGraph(); }

    GraphPlot *plot() const { return m_view->plot(); }

    void onMouseDoubleClick(QMouseEvent *event);
    void onMouseDragBegin(QMouseEvent *event);
    void onMouseDragMove(QMouseEvent *event);
    void onMouseDragEnd(QMouseEvent *event);

    void cancelMousePressAndDragging();

private:
    void setupUi(int graphId);

    void setupWindowOptions();

    void updateWindowOptions(bool onlyFlags = false);
    void updateWindowFlags(const IniGraph &ini);
    void updateBorder(const IniGraph &ini);

    void setupHoverTimer();

    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_mouseDragResize = false;
    bool m_iniMaximized = false;

    GraphView *m_view = nullptr;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QRect m_iniGeometry;

    QTimer m_hoverTimer;
};

#endif // GRAPHWINDOW_H
