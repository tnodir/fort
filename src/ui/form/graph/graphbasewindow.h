#ifndef GRAPHBASEWINDOW_H
#define GRAPHBASEWINDOW_H

#include <QTimer>

#include <form/controls/formwindow.h>
#include <form/graph/graph_types.h>
#include <form/graph/graphview.h>

class GraphPopup;

class GraphBaseWindow : public FormWindow
{
    Q_OBJECT

protected:
    explicit GraphBaseWindow(int graphId, QWidget *parent = nullptr);

public:
    ~GraphBaseWindow() override;

    int graphId() const { return iniGraph().graphId(); }

    virtual bool inTaskbar() const = 0;

    bool isHostChanged() const override;

    WindowCode windowCode() const override { return WindowGraph; }
    QString helpSection() const override { return "traffic-graph"; }
    QString iniGroup() const override;
    bool deleteOnClose() const override;

    void saveWindowState(bool wasVisible) override;

    // The graph's frame in the screen's coordinates: a taskbar's graph is a native child window
    QRect globalFrameGeometry() const;

    // The sides of the window to place the popup at, by priority
    virtual QList<GraphSide> popupSides() const = 0;
    virtual bool popupOnTop() const = 0;

    GraphPlot *plot() const { return m_view->plot(); }

protected:
    GraphView *view() const { return m_view; }

    const IniGraph &iniGraph() const { return m_view->iniGraph(); }
    IniGraph &iniGraph() { return m_view->iniGraph(); }

    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    // Polls the cursor till it is out of the graph's frame: for a leave with the cursor still in
    // it, and for a window hidden under the cursor
    void startCursorLeaveWatch();
    void stopCursorLeaveWatch();
    bool isCursorLeaveWatched() const { return m_cursorLeaveTimer.isActive(); }

    virtual void onCursorLeft();

private:
    void setupUi(int graphId);
    void setupCursorLeaveTimer();

    void showPopup();
    void closePopup();

    bool isCursorInFrame() const;
    void checkCursorLeave();

private:
    GraphView *m_view = nullptr;
    GraphPopup *m_popup = nullptr;

    QTimer m_cursorLeaveTimer;
};

#endif // GRAPHBASEWINDOW_H
