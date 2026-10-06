#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include <form/controls/formwindow.h>
#include <form/graph/graphplot.h>

class IniUser;

class GraphWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowGraph; }
    bool deleteOnClose() const override;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

signals:
    void mouseRightClick(QMouseEvent *event);

public slots:
    void addTraffic(qint64 unixTime, quint64 inBytes, quint64 outBytes);

private slots:
    void checkHoverLeave();

    void updateGraph();

protected:
    static GraphPlot::ColorArray getColors(const IniUser &ini);

private:
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
    void updateWindowFlags(const IniUser &ini);
    void updateColors(const IniUser &ini);
    void updateFonts(const IniUser &ini);
    void updateFormat(const IniUser &ini);

    void setupTimer();
    void startUpdateTimer();

    void updateSpeed(qint64 unixTime);
    QString getSpeedText(qint64 unixTime) const;

    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

protected:
    void showEvent(QShowEvent *event) override;

    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_mouseDragResize = false;

    GraphPlot *m_plot = nullptr;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QTimer m_updateTimer;
    QTimer m_hoverTimer;
};

#endif // GRAPHWINDOW_H
