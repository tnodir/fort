#ifndef WINDOWSTATEWATCHER_H
#define WINDOWSTATEWATCHER_H

#include "basewindowstatewatcher.h"

class WindowStateWatcher : public BaseWindowStateWatcher
{
    Q_OBJECT

public:
    explicit WindowStateWatcher(QObject *parent = nullptr);

    void restore(QWindow *window, const QSize &defaultSize, const QRect &rect, bool maximized);

public slots:
    void install(QWindow *window);

private:
    void handleWindowPositionChange(QWindow *window);
    void handleWindowSizeChange(QWindow *window);
    void handleWindowVisibilityChange(QWindow *window);

private slots:
    void onPositionChanged();
    void onSizeChanged();
    void onVisibilityChanged();
};

#endif // WINDOWSTATEWATCHER_H
