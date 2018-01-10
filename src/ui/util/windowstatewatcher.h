#ifndef WINDOWSTATEWATCHER_H
#define WINDOWSTATEWATCHER_H

#include <QObject>
#include <QRect>

QT_FORWARD_DECLARE_CLASS(QWindow)

class WindowStateWatcher : public QObject
{
    Q_OBJECT

public:
    explicit WindowStateWatcher(QObject *parent = nullptr);

    bool maximized() const { return m_maximized; }
    void setMaximized(bool maximized) { m_maximized = maximized; }

    QRect geometry() const { return m_rect; }
    void setGeometry(const QRect &rect) { m_rect = m_rectPrev = rect; }

signals:

public slots:
    void setup(QWindow *window);

private slots:
    void onRectChanged();
    void onVisibilityChanged();

private:
    bool m_maximized;

    QRect m_rect;
    QRect m_rectPrev;
};

#endif // WINDOWSTATEWATCHER_H
