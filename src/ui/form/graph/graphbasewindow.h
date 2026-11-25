#ifndef GRAPHBASEWINDOW_H
#define GRAPHBASEWINDOW_H

#include <form/controls/formwindow.h>
#include <form/graph/graphview.h>

class GraphBaseWindow : public FormWindow
{
    Q_OBJECT

protected:
    explicit GraphBaseWindow(int graphId, QWidget *parent = nullptr);

public:
    int graphId() const { return iniGraph().graphId(); }

    virtual bool inTaskbar() const = 0;

    bool isHostChanged() const override;

    WindowCode windowCode() const override { return WindowGraph; }
    QString helpSection() const override { return "traffic-graph"; }
    QString iniGroup() const override;
    bool deleteOnClose() const override;

    void saveWindowState(bool wasVisible) override;

protected:
    GraphView *view() const { return m_view; }
    GraphPlot *plot() const { return m_view->plot(); }

    const IniGraph &iniGraph() const { return m_view->iniGraph(); }
    IniGraph &iniGraph() { return m_view->iniGraph(); }

private:
    void setupUi(int graphId);

private:
    GraphView *m_view = nullptr;
};

#endif // GRAPHBASEWINDOW_H
