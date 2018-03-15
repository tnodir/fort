#ifndef BASEWINDOWSTATEWATCHER_H
#define BASEWINDOWSTATEWATCHER_H

#include <QWindow>

class BaseWindowStateWatcher : public QObject
{
    Q_OBJECT

public:
    explicit BaseWindowStateWatcher(QObject *parent = nullptr);

    bool maximized() const { return m_maximized; }
    void setMaximized(bool maximized) { m_maximized = maximized; }

    QRect geometry() const { return m_rect; }
    void setGeometry(const QRect &rect) { m_rect = m_rectPrev = rect; }

    void reset(const QRect &rect, bool maximized) {
        setGeometry(rect);
        setMaximized(maximized);
    }

signals:

public slots:
    void uninstall(QObject *window);

protected:
    void handleRectChange(const QRect &rect, QWindow::Visibility visibility);
    void handleVisibilityChange(QWindow::Visibility visibility);

private:
    bool m_maximized;

    QRect m_rect;
    QRect m_rectPrev;
};

#endif // BASEWINDOWSTATEWATCHER_H
