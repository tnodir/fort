#ifndef WIDGETWINDOW_H
#define WIDGETWINDOW_H

#include <QWidget>

class WidgetWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetWindow(QWidget *parent = nullptr, Qt::WindowFlags f = {});

    void showWindow(bool activate = true, bool expose = false);

    void centerTo(QWidget *w);
    void centerTo(QScreen *s);

    void moveNextTo(QWidget *w, int spacing = 10);

    static void showWidget(QWidget *w, bool activate = true, bool expose = false);
    static void excludeWindowFromCapture(QWidget *w, bool exclude = true);

signals:
    void activationChanged(bool isActive);

    void positionChanged();
    void sizeChanged();

    void visibilityChanged(bool isVisible);

    void aboutToShow();
    void aboutToClose();

    void defaultKeyPressed();

protected slots:
    virtual bool checkAboutToClose() { return true; }

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;

    void ensureWindowScreenBounds();

private:
    static void exposeWidget(QWidget *w);
};

#endif // WIDGETWINDOW_H
