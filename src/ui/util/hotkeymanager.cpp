#include "hotkeymanager.h"

#include <QAction>
#include <QKeySequence>

#include "nativeeventfilter.h"

HotKeyManager::HotKeyManager(NativeEventFilter *nativeEventFilter, QObject *parent) :
    QObject(parent), m_nativeEventFilter(nativeEventFilter)
{
    connect(m_nativeEventFilter, &NativeEventFilter::hotKeyPressed, this,
            &HotKeyManager::onHotKeyPressed);
}

void HotKeyManager::setEnabled(bool v)
{
    if (m_enabled != v) {
        m_enabled = v;
        updateActions();
    }
}

bool HotKeyManager::addAction(QAction *action, const QKeySequence &shortcut)
{
    action->setText(action->text());

    action->setShortcut(shortcut);
    action->setShortcutVisibleInContextMenu(false);

    const int hotKeyId = m_actions.size();
    action->setData(hotKeyId);

    m_actions.append(action);

    return true;
}

void HotKeyManager::removeActions()
{
    m_nativeEventFilter->unregisterHotKeys();

    m_actions.clear();
}

void HotKeyManager::updateActions()
{
    m_nativeEventFilter->unregisterHotKeys();

    for (QAction *action : qAsConst(m_actions)) {
        action->setShortcutVisibleInContextMenu(enabled());
        if (enabled()) {
            registerHotKey(action);
        }
    }
}

void HotKeyManager::onHotKeyPressed(int hotKeyId)
{
    if (hotKeyId >= m_actions.size())
        return;

    QAction *action = m_actions.at(hotKeyId);
    if (action->isEnabled()) {
        action->trigger();
    }
}

void HotKeyManager::registerHotKey(QAction *action) const
{
    const QKeySequence shortcut = action->shortcut();
    const int hotKeyId = action->data().toInt();

    const auto keyCombination = shortcut[0];
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int key = keyCombination;
#else
    const int key = keyCombination.toCombined();
#endif

    m_nativeEventFilter->registerHotKey(hotKeyId, key);
}
