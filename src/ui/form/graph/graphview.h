#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QTimer>
#include <QWidget>

#include <form/graph/graphplot.h>
#include <user/inigraph.h>

class GraphView : public QWidget
{
    Q_OBJECT

public:
    explicit GraphView(int graphId, QWidget *parent = nullptr);

    const IniGraph &iniGraph() const { return m_iniGraph; }
    IniGraph &iniGraph() { return m_iniGraph; }

    GraphPlot *plot() const { return m_plot; }

signals:
    void optionsUpdated();
    void speedUpdated(const QString &inText, const QString &outText);

public slots:
    void addTraffic(qint64 unixTime, quint64 inBytes, quint64 outBytes);

    void updateGraph();

private:
    void setupUi();

    void setupOptions();

    void forceUpdateOptions();
    void updateOptions(bool onlyFlags = false);
    void updateColors(const IniGraph &ini);
    void updateFonts(const IniGraph &ini);
    void updateFormat(const IniGraph &ini);
    void updateSpeedLayout(const IniGraph &ini);

    void setupTimer();
    void startUpdateTimer();

    void updateSpeed(qint64 unixTime);

    static GraphPlot::ColorArray getColors(const IniGraph &ini);

protected:
    void showEvent(QShowEvent *event) override;

private:
    GraphPlot *m_plot = nullptr;

    IniGraph m_iniGraph;

    QTimer m_updateTimer;
};

#endif // GRAPHVIEW_H
