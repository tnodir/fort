#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QAction>
#include <QList>

QT_FORWARD_DECLARE_CLASS(NativeEventFilter)

class HotKeyManager : public QObject
{
    Q_OBJECT

public:
    explicit HotKeyManager(NativeEventFilter *nativeEventFilter, QObject *parent = nullptr);

    bool addAction(QAction *action, const QKeySequence &shortcut);

    void removeActions();

signals:

public slots:

private slots:
    void onHotKeyPressed(int hotKeyId);

private:
    NativeEventFilter *m_nativeEventFilter = nullptr;

    QList<QAction *> m_actions;
};

#endif // HOTKEYMANAGER_H
