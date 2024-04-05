#ifndef HOTKEYMANAGER_H
#define HOTKEYMANAGER_H

#include <QAction>
#include <QList>

#include <util/ioc/iocservice.h>

class NativeEventFilter;

class HotKeyManager : public QObject, public IocService
{
    Q_OBJECT

public:
    explicit HotKeyManager(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    bool global() const { return m_global; }

    const QList<QAction *> &actions() const { return m_actions; }

    void initialize(bool enabled, bool global);

    void setUp() override;
    void tearDown() override;

    bool addAction(QAction *action);

    void removeActions();

private slots:
    void onHotKeyPressed(int hotKeyId);

private:
    void updateActions();

    bool checkShortcutsChanged() const;

private:
    bool m_enabled = false;
    bool m_global = false;

    QList<QAction *> m_actions;
    QVector<QKeyCombination> m_keys;
};

#endif // HOTKEYMANAGER_H
