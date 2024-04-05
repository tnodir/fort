#include "nativeeventfilter.h"

#include <QCoreApplication>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <dbt.h>
#include <wtsapi32.h>

namespace {

const QHash<Qt::Key, quint8> g_keyTbl = {
    { Qt::Key_Escape, VK_ESCAPE },
    { Qt::Key_Escape, VK_ESCAPE },
    { Qt::Key_Tab, VK_TAB },
    { Qt::Key_Backtab, VK_TAB },
    { Qt::Key_Backspace, VK_BACK },
    { Qt::Key_Return, VK_RETURN },
    { Qt::Key_Enter, VK_RETURN },
    { Qt::Key_Insert, VK_INSERT },
    { Qt::Key_Delete, VK_DELETE },
    { Qt::Key_Pause, VK_PAUSE },
    { Qt::Key_Print, VK_PRINT },
    { Qt::Key_Clear, VK_CLEAR },
    { Qt::Key_Home, VK_HOME },
    { Qt::Key_End, VK_END },
    { Qt::Key_Left, VK_LEFT },
    { Qt::Key_Up, VK_UP },
    { Qt::Key_Right, VK_RIGHT },
    { Qt::Key_Down, VK_DOWN },
    { Qt::Key_PageUp, VK_PRIOR },
    { Qt::Key_PageDown, VK_NEXT },
    { Qt::Key_CapsLock, VK_CAPITAL },
    { Qt::Key_NumLock, VK_NUMLOCK },
    { Qt::Key_ScrollLock, VK_SCROLL },

    { Qt::Key_F1, VK_F1 },
    { Qt::Key_F2, VK_F2 },
    { Qt::Key_F3, VK_F3 },
    { Qt::Key_F4, VK_F4 },
    { Qt::Key_F5, VK_F5 },
    { Qt::Key_F6, VK_F6 },
    { Qt::Key_F7, VK_F7 },
    { Qt::Key_F8, VK_F8 },
    { Qt::Key_F9, VK_F9 },
    { Qt::Key_F10, VK_F10 },
    { Qt::Key_F11, VK_F11 },
    { Qt::Key_F12, VK_F12 },
    { Qt::Key_F13, VK_F13 },
    { Qt::Key_F14, VK_F14 },
    { Qt::Key_F15, VK_F15 },
    { Qt::Key_F16, VK_F16 },
    { Qt::Key_F17, VK_F17 },
    { Qt::Key_F18, VK_F18 },
    { Qt::Key_F19, VK_F19 },
    { Qt::Key_F20, VK_F20 },
    { Qt::Key_F21, VK_F21 },
    { Qt::Key_F22, VK_F22 },
    { Qt::Key_F23, VK_F23 },
    { Qt::Key_F24, VK_F24 },

    { Qt::Key_Menu, VK_APPS },
    { Qt::Key_Help, VK_HELP },

    { Qt::Key_Back, VK_BROWSER_BACK },
    { Qt::Key_Forward, VK_BROWSER_FORWARD },
    { Qt::Key_Stop, VK_BROWSER_STOP },
    { Qt::Key_Refresh, VK_BROWSER_REFRESH },
    { Qt::Key_VolumeDown, VK_VOLUME_DOWN },
    { Qt::Key_VolumeMute, VK_VOLUME_MUTE },
    { Qt::Key_VolumeUp, VK_VOLUME_UP },

    { Qt::Key_MediaPlay, VK_MEDIA_PLAY_PAUSE },
    { Qt::Key_MediaStop, VK_MEDIA_STOP },
    { Qt::Key_MediaPrevious, VK_MEDIA_PREV_TRACK },
    { Qt::Key_MediaNext, VK_MEDIA_NEXT_TRACK },

    { Qt::Key_HomePage, VK_BROWSER_HOME },
    { Qt::Key_Favorites, VK_BROWSER_FAVORITES },
    { Qt::Key_Search, VK_BROWSER_SEARCH },

    { Qt::Key_LaunchMail, VK_LAUNCH_MAIL },
    { Qt::Key_LaunchMedia, VK_LAUNCH_MEDIA_SELECT },
    { Qt::Key_Launch0, VK_LAUNCH_APP1 },
    { Qt::Key_Launch1, VK_LAUNCH_APP2 },

    { Qt::Key_Mode_switch, VK_MODECHANGE },
    { Qt::Key_Select, VK_SELECT },

    { Qt::Key_Cancel, VK_CANCEL },
    { Qt::Key_Printer, VK_PRINT },
    { Qt::Key_Execute, VK_EXECUTE },
    { Qt::Key_Sleep, VK_SLEEP },
    { Qt::Key_Play, VK_PLAY },
    { Qt::Key_Zoom, VK_ZOOM },
};

}

NativeEventFilter::NativeEventFilter(QObject *parent) : QObject(parent)
{
    qApp->installNativeEventFilter(this);
}

NativeEventFilter::~NativeEventFilter()
{
    qApp->removeNativeEventFilter(this);
}

bool NativeEventFilter::registerHotKey(
        int hotKeyId, Qt::Key keyCode, Qt::KeyboardModifiers modifiers)
{
    Q_ASSERT(uint(hotKeyId) <= 0xBFFF);

    const quint32 nativeMod = nativeModifiers(modifiers);
    const quint32 nativeKey = nativeKeyCode(keyCode);

    const quint32 nativeModifiers = nativeMod | autoRepeatModifier(/*autoRepeat=*/true);

    if (!RegisterHotKey(nullptr, 0, nativeModifiers, nativeKey))
        return false;

    setKeyId(hotKeyId, nativeMod, nativeKey);

    return true;
}

bool NativeEventFilter::registerHotKey(int hotKeyId, QKeyCombination key)
{
    return registerHotKey(hotKeyId, Qt::Key(key & ~Qt::KeyboardModifierMask),
            Qt::KeyboardModifiers(key & Qt::KeyboardModifierMask));
}

void NativeEventFilter::unregisterHotKeys()
{
    if (m_keyIdMap.isEmpty())
        return;

    for (const int hotKeyId : std::as_const(m_keyIdMap)) {
        UnregisterHotKey(nullptr, hotKeyId);
    }

    m_keyIdMap.clear();
}

bool NativeEventFilter::registerSessionNotification(quintptr winId)
{
    return WTSRegisterSessionNotification((HWND) winId, NOTIFY_FOR_THIS_SESSION);
}

void NativeEventFilter::unregisterSessionNotification(quintptr winId)
{
    WTSUnRegisterSessionNotification((HWND) winId);
}

void NativeEventFilter::setKeyId(int hotKeyId, quint32 nativeMod, quint32 nativeKey)
{
    const quint32 nativeKeyMod = nativeMod | (nativeKey << 16);

    m_keyIdMap.insert(nativeKeyMod, hotKeyId);
}

bool NativeEventFilter::removeKeyId(int hotKeyId)
{
    const quint32 nativeKeyMod = m_keyIdMap.key(hotKeyId);

    return m_keyIdMap.remove(nativeKeyMod);
}

int NativeEventFilter::getKeyId(quint32 nativeMod, quint32 nativeKey) const
{
    const quint32 nativeKeyMod = nativeMod | (nativeKey << 16);

    return m_keyIdMap.value(nativeKeyMod, -1);
}

void NativeEventFilter::processWmHotKey(void *message)
{
    const MSG *msg = static_cast<MSG *>(message);

    const int hotKeyId = getKeyId(LOWORD(msg->lParam), HIWORD(msg->lParam));

    if (hotKeyId >= 0) {
        emit hotKeyPressed(hotKeyId);
    }
}

void NativeEventFilter::processWmSettingChange(void *message)
{
    const MSG *msg = static_cast<MSG *>(message);

    const auto src = reinterpret_cast<const wchar_t *>(msg->lParam);

    if (src && wcscmp(src, L"Environment") == 0) {
        emit environmentChanged();
    }
}

void NativeEventFilter::processWmWtsSessionChange(void *message)
{
    const MSG *msg = static_cast<MSG *>(message);

    if (msg->wParam == WTS_SESSION_LOCK) {
        emit sessionLocked();
    }
}

void NativeEventFilter::processWmDeviceChange(void *message)
{
    const MSG *msg = static_cast<MSG *>(message);

    switch (msg->wParam) {
    case DBT_DEVICEARRIVAL:
    case DBT_DEVICEREMOVECOMPLETE:
        emit driveListChanged();
    }
}

bool NativeEventFilter::nativeEventFilter(
        const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    const MSG *msg = static_cast<MSG *>(message);

    switch (msg->message) {
    case WM_HOTKEY: {
        processWmHotKey(message);
    } break;
    case WM_SETTINGCHANGE: {
        processWmSettingChange(message);
    } break;
    case WM_WTSSESSION_CHANGE: {
        processWmWtsSessionChange(message);
    } break;
    case WM_DEVICECHANGE: {
        processWmDeviceChange(message);
    } break;
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

    return g_keyTbl.value(keyCode, 0);
}

quint32 NativeEventFilter::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    return ((modifiers & Qt::ShiftModifier) ? MOD_SHIFT : 0)
            | ((modifiers & Qt::ControlModifier) ? MOD_CONTROL : 0)
            | ((modifiers & Qt::AltModifier) ? MOD_ALT : 0)
            | ((modifiers & Qt::MetaModifier) ? MOD_WIN : 0);
}

quint32 NativeEventFilter::autoRepeatModifier(bool autoRepeat)
{
    return (autoRepeat ? 0 : MOD_NOREPEAT);
}
