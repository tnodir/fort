#ifndef WIDGETWINDOW_H
#define WIDGETWINDOW_H

#include <QWidget>

class WidgetWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetWindow(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    virtual bool deleteOnClose() const { return true; }

    void showWindow(bool activate = true);
    void exposeWindow();

    void centerTo(QWidget *w);

    static void showWidget(QWidget *w, bool activate = true);
    static void exposeWidget(QWidget *w);
    static void excludeWindowFromCapture(QWidget *w, bool exclude = true);

signals:
    void activationChanged(bool isActive);

    void positionChanged();
    void sizeChanged();

    void visibilityChanged(bool isVisible);

    void aboutToShow();
    void aboutToClose(QEvent *event);

    void defaultKeyPressed();

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
