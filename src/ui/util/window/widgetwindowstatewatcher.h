#ifndef WIDGETWINDOWSTATEWATCHER_H
#define WIDGETWINDOWSTATEWATCHER_H

#include "basewindowstatewatcher.h"

class WidgetWindow;

class WidgetWindowStateWatcher : public BaseWindowStateWatcher
{
    Q_OBJECT

public:
    explicit WidgetWindowStateWatcher(QObject *parent = nullptr);

    void restore(WidgetWindow *window, const QSize &defaultSize, const QRect &rect, bool maximized);

public slots:
    void install(WidgetWindow *window);

private slots:
    void onPositionChanged();
    void onSizeChanged();
    void onVisibilityChanged();

private:
    void handleWindowPositionChange(WidgetWindow *window);
    void handleWindowSizeChange(WidgetWindow *window);
    void handleWindowVisibilityChange(WidgetWindow *window);

private:
    static QWindow::Visibility getVisibility(WidgetWindow *window);
};

#endif // WIDGETWINDOWSTATEWATCHER_H
