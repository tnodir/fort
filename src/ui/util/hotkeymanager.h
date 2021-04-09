#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QAction>
#include <QList>

class NativeEventFilter;

class HotKeyManager : public QObject
{
    Q_OBJECT

public:
    explicit HotKeyManager(NativeEventFilter *nativeEventFilter, QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);

    bool addAction(QAction *action, const QKeySequence &shortcut);

    void removeActions();

private slots:
    void onHotKeyPressed(int hotKeyId);

private:
    void updateActions();

    void registerHotKey(QAction *action) const;

private:
    bool m_enabled = false;

    NativeEventFilter *m_nativeEventFilter = nullptr;

    QList<QAction *> m_actions;
};

#endif // HOTKEYMANAGER_H
