#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include <form/controls/formwindow.h>
#include <form/graph/graphplot.h>
#include <user/inigraph.h>

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

signals:
    void mouseRightClick(QMouseEvent *event);

public slots:
    void addTraffic(qint64 unixTime, quint64 inBytes, quint64 outBytes);

private slots:
    void checkHoverLeave();

    void updateWindowGeometry();

    void updateGraph();

protected:
    static GraphPlot::ColorArray getColors(const IniGraph &ini);

private:
    const IniGraph &iniGraph() const { return m_iniGraph; }
    IniGraph &iniGraph() { return m_iniGraph; }

    void onMouseDoubleClick(QMouseEvent *event);
    void onMouseDragBegin(QMouseEvent *event);
    void onMouseDragMove(QMouseEvent *event);
    void onMouseDragEnd(QMouseEvent *event);

    void cancelMousePressAndDragging();

private:
    void setupUi();

    void setupFlagsAndColors();

    void forceUpdateFlagsAndColors();
    void updateFlagsAndColors(bool onlyFlags = false);
    void updateWindowFlags(const IniGraph &ini);
    void updateColors(const IniGraph &ini);
    void updateFonts(const IniGraph &ini);
    void updateFormat(const IniGraph &ini);

    void setupTimer();
    void startUpdateTimer();

    void updateSpeed(qint64 unixTime);

    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

protected:
    void showEvent(QShowEvent *event) override;

    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_mouseDragResize = false;
    bool m_iniMaximized = false;

    GraphPlot *m_plot = nullptr;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QRect m_iniGeometry;

    IniGraph m_iniGraph;

    QTimer m_updateTimer;
    QTimer m_hoverTimer;
};

#endif // GRAPHWINDOW_H
