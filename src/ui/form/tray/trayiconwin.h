#ifndef TRAYICONWIN_H
#define TRAYICONWIN_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QSystemTrayIcon>

QT_FORWARD_DECLARE_CLASS(QWindow)

class TrayIconWin : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit TrayIconWin(QObject *parent = nullptr);
    ~TrayIconWin() override;

    void setIcon(const QIcon &icon);
    void setToolTip(const QString &toolTip);
    void setVisible(bool visible);
    bool isVisible() const { return m_visible; }
    void showMessage(const QString &title, const QString &message);

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void activated(QSystemTrayIcon::ActivationReason reason);
    void contextMenuRequested(const QPoint &pos);
    void messageClicked();

private:
    void addIcon();
    void removeIcon();
    void processTaskbarCreated(quintptr window);
    void processTrayEvent(quint32 event, quintptr eventParam);
    void updateIcon();
    void updateToolTip();

    bool m_visible = false;
    bool m_registered = false;
    bool m_ignoreNextMouseRelease = false;

    QWindow *m_window = nullptr;
    quintptr m_taskbarCreated = 0;
    void *m_icon = nullptr;

    QIcon m_sourceIcon;
    QString m_toolTip;
};

#endif // TRAYICONWIN_H
