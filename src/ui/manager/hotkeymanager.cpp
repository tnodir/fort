#include "hotkeymanager.h"

#include <QAction>
#include <QKeySequence>

#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "nativeeventfilter.h"

namespace {

QKeyCombination getActionKey(QAction *action)
{
    const QKeySequence keySequence = action->shortcut();

    return keySequence.isEmpty() ? QKeyCombination() : keySequence[0];
}

}

HotKeyManager::HotKeyManager(QObject *parent) : QObject(parent) { }

void HotKeyManager::initialize(bool enabled, bool global)
{
    if (m_enabled == enabled && m_global == global) {
        if (!checkShortcutsChanged())
            return;
    }

    m_enabled = enabled;
    m_global = global;

    updateActions();
}

void HotKeyManager::setUp()
{
    auto nativeEventFilter = IoCDependency<NativeEventFilter>();

    connect(nativeEventFilter, &NativeEventFilter::hotKeyPressed, this,
            &HotKeyManager::onHotKeyPressed);
}

void HotKeyManager::tearDown()
{
    disconnect(IoC<NativeEventFilter>());

    removeActions();
}

bool HotKeyManager::addAction(QAction *action)
{
    action->setShortcutVisibleInContextMenu(false);

    m_actions.append(action);
    m_keys.append(getActionKey(action));

    return true;
}

void HotKeyManager::removeActions()
{
    IoC<NativeEventFilter>()->unregisterHotKeys();

    m_actions.clear();
    m_keys.clear();
}

void HotKeyManager::updateActions()
{
    auto eventFilter = IoC<NativeEventFilter>();

    eventFilter->unregisterHotKeys();

    int hotKeyId = 0;
    for (QAction *action : std::as_const(m_actions)) {
        action->setShortcutVisibleInContextMenu(enabled());

        const auto key = getActionKey(action);
        m_keys[hotKeyId] = key;

        if (enabled() && global()) {
            eventFilter->registerHotKey(hotKeyId, key);
        }

        ++hotKeyId;
    }
}

bool HotKeyManager::checkShortcutsChanged() const
{
    int hotKeyId = 0;
    for (QAction *action : std::as_const(m_actions)) {
        const auto oldKey = getActionKey(action);
        const auto key = m_keys[hotKeyId];

        if (key != oldKey)
            return true;

        ++hotKeyId;
    }

    return false;
}

void HotKeyManager::onHotKeyPressed(int hotKeyId)
{
    if (WindowManager::activateModalWidget())
        return;

    QAction *action = m_actions.value(hotKeyId);
    if (action && action->isEnabled()) {
        action->trigger();
        OsUtil::beep();
    }
}
