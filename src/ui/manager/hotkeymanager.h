#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QAction>
#include <QList>

#include "../util/ioc/iocservice.h"

class NativeEventFilter;

class HotKeyManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit HotKeyManager(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool v);

    void setUp() override;
    void tearDown() override;

    bool addAction(QAction *action, const QKeySequence &shortcut);

    void removeActions();

private slots:
    void onHotKeyPressed(int hotKeyId);

private:
    void updateActions();

    void registerHotKey(QAction *action) const;

private:
    bool m_enabled = false;

    QList<QAction *> m_actions;
};

#endif // HOTKEYMANAGER_H
