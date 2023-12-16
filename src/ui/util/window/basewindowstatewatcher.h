#ifndef BASEWINDOWSTATEWATCHER_H
#define BASEWINDOWSTATEWATCHER_H

#include <QWindow>

class BaseWindowStateWatcher : public QObject
{
    Q_OBJECT

public:
    explicit BaseWindowStateWatcher(QObject *parent = nullptr);

    bool visible() const { return m_visible; }

    bool maximized() const { return m_maximized; }
    void setMaximized(bool maximized) { m_maximized = maximized; }

    QRect geometry() const;
    void setGeometry(const QRect &rect);

    void reset(const QRect &rect, bool maximized);

public slots:
    void uninstall(QObject *window);

protected:
    void handlePositionChange(const QPoint &pos, QWindow::Visibility visibility);
    void handleSizeChange(const QSize &size, QWindow::Visibility visibility);
    void handleVisibilityChange(QWindow::Visibility visibility);

private:
    bool m_visible : 1 = false;
    bool m_maximized : 1 = false;

    QPoint m_pos;
    QPoint m_posPrev;

    QSize m_size;
    QSize m_sizePrev;
};

#endif // BASEWINDOWSTATEWATCHER_H
