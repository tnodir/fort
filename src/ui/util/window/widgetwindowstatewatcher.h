#ifndef WIDGETWINDOWSTATEWATCHER_H
#define WIDGETWINDOWSTATEWATCHER_H

#include "basewindowstatewatcher.h"

QT_FORWARD_DECLARE_CLASS(WidgetWindow)

class WidgetWindowStateWatcher : public BaseWindowStateWatcher
{
    Q_OBJECT

public:
    explicit WidgetWindowStateWatcher(QObject *parent = nullptr);

    void restore(WidgetWindow *window, const QSize &defaultSize,
                 const QRect &rect, bool maximized);

signals:

public slots:
    void install(WidgetWindow *window);

private slots:
    void onRectChanged();
    void onVisibilityChanged();

private:
    static QWindow::Visibility getVisibility(WidgetWindow *window);
};

#endif // WIDGETWINDOWSTATEWATCHER_H
