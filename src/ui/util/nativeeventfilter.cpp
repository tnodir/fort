#include "nativeeventfilter.h"

#include <QCoreApplication>
#include <QKeySequence>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

NativeEventFilter::NativeEventFilter(QObject *parent) :
    QObject(parent)
{
    qApp->installNativeEventFilter(this);
}

NativeEventFilter::~NativeEventFilter()
{
    qApp->removeNativeEventFilter(this);
}

bool NativeEventFilter::registerHotKey(int hotKeyId,
                                       Qt::Key keyCode,
                                       Qt::KeyboardModifiers modifiers,
                                       bool autoRepeat)
{
    Q_ASSERT(uint(hotKeyId) <= 0xBFFF);

    const quint32 nativeMod = nativeModifiers(modifiers, autoRepeat);
    const quint32 nativeKey = nativeKeyCode(keyCode);

    if (!RegisterHotKey(nullptr, 0, nativeMod, nativeKey))
        return false;

    setKeyId(hotKeyId, nativeMod, nativeKey);

    return true;
}

bool NativeEventFilter::registerHotKey(int hotKeyId,
                                       const QKeySequence &shortcut,
                                       bool autoRepeat)
{
    const int key = shortcut[0];

    return registerHotKey(hotKeyId,
                          Qt::Key(key & ~Qt::KeyboardModifierMask),
                          Qt::KeyboardModifiers(key & Qt::KeyboardModifierMask),
                          autoRepeat);
}

void NativeEventFilter::unregisterHotKey(int hotKeyId)
{
    UnregisterHotKey(nullptr, hotKeyId);

    removeKeyId(hotKeyId);
}

void NativeEventFilter::unregisterHotKeys()
{
    foreach (int hotKeyId, m_keyIdMap) {
        UnregisterHotKey(nullptr, hotKeyId);
    }

    m_keyIdMap.clear();
}

void NativeEventFilter::setKeyId(int hotKeyId, quint32 nativeMod,
                                 quint32 nativeKey)
{
    const quint32 nativeKeyMod = nativeMod | (nativeKey << 16);

    m_keyIdMap.insert(nativeKeyMod, hotKeyId);
}

void NativeEventFilter::removeKeyId(int hotKeyId)
{
    const quint32 nativeKeyMod = m_keyIdMap.key(hotKeyId);

    m_keyIdMap.remove(nativeKeyMod);
}

int NativeEventFilter::getKeyId(quint32 nativeMod, quint32 nativeKey) const
{
    const quint32 nativeKeyMod = nativeMod | (nativeKey << 16);

    return m_keyIdMap.value(nativeKeyMod, -1);
}

bool NativeEventFilter::nativeEventFilter(const QByteArray &eventType,
                                          void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    const MSG *msg = static_cast<MSG *>(message);

    if (msg->message == WM_HOTKEY) {
        const int hotKeyId = getKeyId(LOWORD(msg->lParam), HIWORD(msg->lParam));

        if (hotKeyId >= 0) {
            emit hotKeyPressed(hotKeyId);
        }
    }

    return false;
}

quint32 NativeEventFilter::nativeKeyCode(Qt::Key keyCode)
{
    if (keyCode <= 0xFFFF) {
        const SHORT vk = VkKeyScanW(WCHAR(keyCode));
        if (vk >= 0)
            return LOBYTE(vk);
    }

    switch (keyCode)
    {
    case Qt::Key_Escape:
        return VK_ESCAPE;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return VK_TAB;
    case Qt::Key_Backspace:
        return VK_BACK;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return VK_RETURN;
    case Qt::Key_Insert:
        return VK_INSERT;
    case Qt::Key_Delete:
        return VK_DELETE;
    case Qt::Key_Pause:
        return VK_PAUSE;
    case Qt::Key_Print:
        return VK_PRINT;
    case Qt::Key_Clear:
        return VK_CLEAR;
    case Qt::Key_Home:
        return VK_HOME;
    case Qt::Key_End:
        return VK_END;
    case Qt::Key_Left:
        return VK_LEFT;
    case Qt::Key_Up:
        return VK_UP;
    case Qt::Key_Right:
        return VK_RIGHT;
    case Qt::Key_Down:
        return VK_DOWN;
    case Qt::Key_PageUp:
        return VK_PRIOR;
    case Qt::Key_PageDown:
        return VK_NEXT;
    case Qt::Key_CapsLock:
        return VK_CAPITAL;
    case Qt::Key_NumLock:
        return VK_NUMLOCK;
    case Qt::Key_ScrollLock:
        return VK_SCROLL;

    case Qt::Key_F1:
        return VK_F1;
    case Qt::Key_F2:
        return VK_F2;
    case Qt::Key_F3:
        return VK_F3;
    case Qt::Key_F4:
        return VK_F4;
    case Qt::Key_F5:
        return VK_F5;
    case Qt::Key_F6:
        return VK_F6;
    case Qt::Key_F7:
        return VK_F7;
    case Qt::Key_F8:
        return VK_F8;
    case Qt::Key_F9:
        return VK_F9;
    case Qt::Key_F10:
        return VK_F10;
    case Qt::Key_F11:
        return VK_F11;
    case Qt::Key_F12:
        return VK_F12;
    case Qt::Key_F13:
        return VK_F13;
    case Qt::Key_F14:
        return VK_F14;
    case Qt::Key_F15:
        return VK_F15;
    case Qt::Key_F16:
        return VK_F16;
    case Qt::Key_F17:
        return VK_F17;
    case Qt::Key_F18:
        return VK_F18;
    case Qt::Key_F19:
        return VK_F19;
    case Qt::Key_F20:
        return VK_F20;
    case Qt::Key_F21:
        return VK_F21;
    case Qt::Key_F22:
        return VK_F22;
    case Qt::Key_F23:
        return VK_F23;
    case Qt::Key_F24:
        return VK_F24;

    case Qt::Key_Menu:
        return VK_APPS;
    case Qt::Key_Help:
        return VK_HELP;

    case Qt::Key_Back:
        return VK_BROWSER_BACK;
    case Qt::Key_Forward:
        return VK_BROWSER_FORWARD;
    case Qt::Key_Stop:
        return VK_BROWSER_STOP;
    case Qt::Key_Refresh:
        return VK_BROWSER_REFRESH;
    case Qt::Key_VolumeDown:
        return VK_VOLUME_DOWN;
    case Qt::Key_VolumeMute:
        return VK_VOLUME_MUTE;
    case Qt::Key_VolumeUp:
        return VK_VOLUME_UP;

    case Qt::Key_MediaPlay:
        return VK_MEDIA_PLAY_PAUSE;
    case Qt::Key_MediaStop:
        return VK_MEDIA_STOP;
    case Qt::Key_MediaPrevious:
        return VK_MEDIA_PREV_TRACK;
    case Qt::Key_MediaNext:
        return VK_MEDIA_NEXT_TRACK;

    case Qt::Key_HomePage:
        return VK_BROWSER_HOME;
    case Qt::Key_Favorites:
        return VK_BROWSER_FAVORITES;
    case Qt::Key_Search:
        return VK_BROWSER_SEARCH;

    case Qt::Key_LaunchMail:
        return VK_LAUNCH_MAIL;
    case Qt::Key_LaunchMedia:
        return VK_LAUNCH_MEDIA_SELECT;
    case Qt::Key_Launch0:
        return VK_LAUNCH_APP1;
    case Qt::Key_Launch1:
        return VK_LAUNCH_APP2;

    case Qt::Key_Mode_switch:
        return VK_MODECHANGE;
    case Qt::Key_Select:
        return VK_SELECT;

    case Qt::Key_Cancel:
        return VK_CANCEL;
    case Qt::Key_Printer:
        return VK_PRINT;
    case Qt::Key_Execute:
        return VK_EXECUTE;
    case Qt::Key_Sleep:
        return VK_SLEEP;
    case Qt::Key_Play:
        return VK_PLAY;
    case Qt::Key_Zoom:
        return VK_ZOOM;

    default:
        return 0;
    }
}

quint32 NativeEventFilter::nativeModifiers(Qt::KeyboardModifiers modifiers,
                                           bool autoRepeat)
{
    return (autoRepeat ? MOD_NOREPEAT : 0)
            | ((modifiers & Qt::ShiftModifier) ? MOD_SHIFT : 0)
            | ((modifiers & Qt::ControlModifier) ? MOD_CONTROL : 0)
            | ((modifiers & Qt::AltModifier) ? MOD_ALT : 0)
            | ((modifiers & Qt::MetaModifier) ? MOD_WIN : 0)
            ;
}
