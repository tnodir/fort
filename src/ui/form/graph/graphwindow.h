#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <form/graph/graphbasewindow.h>

class GraphWindow : public GraphBaseWindow
{
    Q_OBJECT

public:
    explicit GraphWindow(int graphId, QWidget *parent = nullptr);

    bool inTaskbar() const override { return false; }

    QList<GraphSide> popupSides() const override;
    bool popupOnTop() const override;

    bool isWindowShown() const override;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

    void onWindowClosed() override;

    void saveWindowGeometry();

private slots:
    void updateWindowGeometry();

    void updateWindowTitle(const QString &inText, const QString &outText);

private:
    void setIniGeometry();

    void onMouseDoubleClick(QMouseEvent *event);
    void onMouseDragBegin(QMouseEvent *event);
    void onMouseDragMove(QMouseEvent *event);
    void onMouseDragEnd(QMouseEvent *event);

    void cancelMousePressAndDragging();

private:
    void setupUi();

    void setupWindowOptions();

    void updateWindowOptions(bool onlyFlags = false);
    void updateWindowFlags(const IniGraph &ini);

    void setWindowOpacityPercent(int percent);

    void checkWindowEdges();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void onCursorLeft() override;

private:
    bool m_mouseDragResize = false;
    bool m_iniMaximized = false;

    QPoint m_mousePressPoint;
    QPoint m_posOnMousePress;
    QSize m_sizeOnMousePress;

    QRect m_iniGeometry;
};

#endif // GRAPHWINDOW_H
