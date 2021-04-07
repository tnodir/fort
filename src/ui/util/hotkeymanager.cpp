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

bool HotKeyManager::addAction(QAction *action, const QKeySequence &shortcut)
{
    const int hotKeyId = m_actions.size();

    const auto keyCombination = shortcut[0];
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int key = keyCombination;
#else
    const int key = keyCombination.toCombined();
#endif

    if (!m_nativeEventFilter->registerHotKey(hotKeyId, key))
        return false;

    action->setText(action->text() + '\t' + shortcut.toString());

    m_actions.append(action);

    return true;
}

void HotKeyManager::removeActions()
{
    m_nativeEventFilter->unregisterHotKeys();

    m_actions.clear();
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
