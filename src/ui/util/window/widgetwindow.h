#ifndef WIDGETWINDOW_H
#define WIDGETWINDOW_H

#include <QWidget>

class WidgetWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetWindow(QWidget *parent = nullptr);

    virtual quint32 windowCode() const { return 0; }

    virtual void saveWindowState(bool wasVisible) { Q_UNUSED(wasVisible); }
    virtual void restoreWindowState() { }

    void showWindow() { showWidget(this); }

    static void showWidget(QWidget *w);

signals:
    void activationChanged(bool isActive);
    void activated();
    void deactivated();

    void positionChanged();
    void sizeChanged();

    void visibilityChanged(bool isVisible);

    void aboutToClose(QEvent *event);

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;
};

#endif // WIDGETWINDOW_H
