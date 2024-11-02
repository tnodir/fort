#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QTimer>

#include <form/controls/formwindow.h>
#include <util/formatutil.h>

class AxisTickerSpeed;

class ConfManager;
class IniUser;
class GraphPlot;
class QCPBars;

class GraphWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowGraph; }
    bool deleteOnClose() const override;

    ConfManager *confManager() const;
    IniUser *iniUser() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

signals:
    void mouseRightClick(QMouseEvent *event);

public slots:
    void addTraffic(qint64 unixTime, quint32 inBytes, quint32 outBytes);

protected slots:
    void setupByIniUser(const IniUser &ini, bool onlyFlags);

private slots:
    void checkHoverLeave();

    void addEmptyTraffic();

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
    void updateFlagsAndColors(const IniUser &ini, bool onlyFlags);
    void updateWindowFlags(const IniUser &ini);
    void updateColors(const IniUser &ini);
    void updateFormat(const IniUser &ini);

    void setupTimer();

    void addData(QCPBars *graph, double rangeLowerKey, double unixTimeKey, quint32 bytes);

    void updateWindowTitleSpeed();
    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

    enum ColorType : qint8 {
        ColorBg = 0,
        ColorIn,
        ColorOut,
        ColorAxis,
        ColorTickLabel,
        ColorLabel,
        ColorGrid,
        ColorCount
    };

    static QVarLengthArray<QColor, GraphWindow::ColorCount> getColors(const IniUser &ini);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    bool m_mouseDragResize = false;

    FormatUtil::SizeFormat m_unitFormat = FormatUtil::SpeedTraditionalFormat;

    qint64 m_lastUnixTime = 0;

    GraphPlot *m_plot = nullptr;
    QSharedPointer<AxisTickerSpeed> m_ticker;
    QCPBars *m_graphIn = nullptr;
    QCPBars *m_graphOut = nullptr;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QTimer m_updateTimer;
    QTimer m_hoverTimer;
};

#endif // GRAPHWINDOW_H
