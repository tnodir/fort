#ifndef WINDOWSTATEWATCHER_H
#define WINDOWSTATEWATCHER_H

#include "basewindowstatewatcher.h"

class WindowStateWatcher : public BaseWindowStateWatcher
{
    Q_OBJECT

public:
    explicit WindowStateWatcher(QObject *parent = nullptr);

    void restore(QWindow *window, const QSize &defaultSize,
                 const QRect &rect, bool maximized);

signals:

public slots:
    void install(QWindow *window);

private slots:
    void onPositionChanged();
    void onSizeChanged();
    void onVisibilityChanged();
};

#endif // WINDOWSTATEWATCHER_H
