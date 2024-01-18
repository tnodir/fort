#include "hotkeymanager.h"

#include <QAction>
#include <QKeySequence>

#include <fortcompat.h>
#include <manager/windowmanager.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>

#include "nativeeventfilter.h"

HotKeyManager::HotKeyManager(QObject *parent) : QObject(parent) { }

void HotKeyManager::initialize(bool enabled, bool global)
{
    if (m_enabled == enabled && m_global == global)
        return;

    m_enabled = enabled;
    m_global = global;

    updateActions();
}

void HotKeyManager::setUp()
{
    auto nativeEventFilter = IoCPinned()->setUpDependency<NativeEventFilter>();

    connect(nativeEventFilter, &NativeEventFilter::hotKeyPressed, this,
            &HotKeyManager::onHotKeyPressed);
}

void HotKeyManager::tearDown()
{
    disconnect(IoC<NativeEventFilter>());
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
    IoC<NativeEventFilter>()->unregisterHotKeys();

    m_actions.clear();
}

void HotKeyManager::updateActions()
{
    auto eventFilter = IoC<NativeEventFilter>();

    eventFilter->unregisterHotKeys();

    for (QAction *action : asConst(m_actions)) {
        action->setShortcutVisibleInContextMenu(enabled());
        if (enabled() && global()) {
            registerHotKey(eventFilter, action);
        }
    }
}

void HotKeyManager::onHotKeyPressed(int hotKeyId)
{
    if (WindowManager::activateModalWidget())
        return;

    if (hotKeyId >= m_actions.size())
        return;

    QAction *action = m_actions.at(hotKeyId);
    if (action->isEnabled()) {
        action->trigger();
        OsUtil::beep();
    }
}

void HotKeyManager::registerHotKey(NativeEventFilter *eventFilter, QAction *action) const
{
    const QKeySequence shortcut = action->shortcut();
    const int hotKeyId = action->data().toInt();

    const auto keyCombination = shortcut[0];
    const int key = keyCombination.toCombined();

    eventFilter->registerHotKey(hotKeyId, key);
}
