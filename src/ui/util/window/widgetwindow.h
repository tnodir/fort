#ifndef WIDGETWINDOW_H
#define WIDGETWINDOW_H

#include <QWidget>

class WidgetWindow : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetWindow(QWidget *parent = nullptr);

    bool isClosing() const { return m_isClosing; }

signals:
    void activationChanged();

    void positionChanged();
    void sizeChanged();

    void visibilityChanged();

    void aboutToClose();

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void changeEvent(QEvent *event) override;

private:
    bool m_isClosing = false;
};

#endif // WIDGETWINDOW_H
