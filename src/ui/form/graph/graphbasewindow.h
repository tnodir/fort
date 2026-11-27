#ifndef GRAPHBASEWINDOW_H
#define GRAPHBASEWINDOW_H

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

private:
    void setupUi(int graphId);

    void showPopup();
    void closePopup();

private:
    GraphView *m_view = nullptr;
    GraphPopup *m_popup = nullptr;
};

#endif // GRAPHBASEWINDOW_H
