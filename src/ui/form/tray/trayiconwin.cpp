#include "trayiconwin.h"

#include <QGuiApplication>
#include <QPixmap>
#include <QWindow>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include <shellapi.h>
#include <windowsx.h>

namespace {

inline constexpr UINT trayCallbackMessage = WM_APP + 101;
inline constexpr GUID trayIconGuid = { 0x7d741c85, 0x8744, 0x4ab6,
    { 0x9c, 0xa2, 0x3f, 0x66, 0x57, 0x4f, 0xb9, 0x12 } };

NOTIFYICONDATAW notifyIconData(HWND window, UINT flags)
{
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = window;
    data.uFlags = flags | NIF_GUID;
    data.uID = 0;
    data.uVersion = NOTIFYICON_VERSION_4;
    data.guidItem = trayIconGuid;

    return data;
}

template<size_t N>
void copyText(const QString &text, wchar_t (&buffer)[N])
{
    const int length = int(qMin(text.size(), qsizetype(N - 1)));
    text.left(length).toWCharArray(buffer);
    buffer[length] = L'\0';
}

}

TrayIconWin::TrayIconWin(QObject *parent) : QObject(parent), m_window(new QWindow)
{
    m_window->setFlags(Qt::Tool | Qt::FramelessWindowHint);
    m_window->create();

    qApp->installNativeEventFilter(this);

    m_taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    if (m_taskbarCreated) {
        ChangeWindowMessageFilterEx(
                HWND(m_window->winId()), UINT(m_taskbarCreated), MSGFLT_ALLOW, nullptr);
    }
}

TrayIconWin::~TrayIconWin()
{
    removeIcon();
    qApp->removeNativeEventFilter(this);
    delete m_window;
    if (m_icon) {
        DestroyIcon(HICON(m_icon));
    }
}

void TrayIconWin::setIcon(const QIcon &icon)
{
    m_sourceIcon = icon;
    updateIcon();
}

void TrayIconWin::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
    updateToolTip();
}

void TrayIconWin::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    if (m_visible) {
        addIcon();
    } else {
        removeIcon();
    }
}

void TrayIconWin::showMessage(const QString &title, const QString &message)
{
    if (!m_registered)
        return;

    auto data = notifyIconData(HWND(m_window->winId()), NIF_INFO);
    copyText(title, data.szInfoTitle);
    copyText(message, data.szInfo);
    data.dwInfoFlags = NIIF_INFO;
    data.uTimeout = 10000;

    Shell_NotifyIconW(NIM_MODIFY, &data);
}

bool TrayIconWin::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);

    const MSG *msg = static_cast<MSG *>(message);
    if (m_taskbarCreated && msg->message == m_taskbarCreated) {
        processTaskbarCreated(quintptr(msg->hwnd));
        return false;
    }

    if (msg->hwnd != HWND(m_window->winId()) || msg->message != trayCallbackMessage)
        return false;

    processTrayEvent(quint32(LOWORD(msg->lParam)), quintptr(msg->wParam));

    if (result)
        *result = 0;

    return true;
}

void TrayIconWin::processTaskbarCreated(quintptr window)
{
    if (m_visible && window == quintptr(m_window->winId())) {
        m_registered = false;
        addIcon();
    }
}

void TrayIconWin::processTrayEvent(quint32 event, quintptr eventParam)
{
    switch (event) {
    case NIN_SELECT:
    case NIN_KEYSELECT:
        if (m_ignoreNextMouseRelease) {
            m_ignoreNextMouseRelease = false;
        } else {
            emit activated(QSystemTrayIcon::Trigger);
        }
        break;
    case WM_LBUTTONDBLCLK:
        m_ignoreNextMouseRelease = true;
        emit activated(QSystemTrayIcon::DoubleClick);
        break;
    case WM_CONTEXTMENU: {
        const WPARAM param = WPARAM(eventParam);
        const QPoint pos(GET_X_LPARAM(param), GET_Y_LPARAM(param));
        emit activated(QSystemTrayIcon::Context);
        SetForegroundWindow(HWND(m_window->winId()));
        emit contextMenuRequested(pos);
    } break;
    case WM_MBUTTONUP:
        emit activated(QSystemTrayIcon::MiddleClick);
        break;
    case NIN_BALLOONUSERCLICK:
        emit messageClicked();
        break;
    default:
        break;
    }
}

void TrayIconWin::addIcon()
{
    if (!m_visible || m_registered || m_sourceIcon.isNull())
        return;

    const HWND window = HWND(m_window->winId());
    auto data = notifyIconData(window, NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP);
    data.uCallbackMessage = trayCallbackMessage;
    data.hIcon = HICON(m_icon);
    copyText(m_toolTip, data.szTip);

    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        Shell_NotifyIconW(NIM_DELETE, &data);
        if (!Shell_NotifyIconW(NIM_ADD, &data))
            return;
    }

    Shell_NotifyIconW(NIM_SETVERSION, &data);
    m_registered = true;
}

void TrayIconWin::removeIcon()
{
    if (!m_registered)
        return;

    auto data = notifyIconData(HWND(m_window->winId()), 0);
    Shell_NotifyIconW(NIM_DELETE, &data);
    m_registered = false;
}

void TrayIconWin::updateIcon()
{
    const QSize size(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    const QPixmap pixmap = m_sourceIcon.pixmap(m_sourceIcon.actualSize(size));
    const HICON icon = pixmap.isNull() ? nullptr : HICON(pixmap.toImage().toHICON());

    if (m_icon) {
        DestroyIcon(HICON(m_icon));
    }
    m_icon = icon;

    if (m_registered) {
        auto data = notifyIconData(HWND(m_window->winId()), NIF_ICON);
        data.hIcon = HICON(m_icon);
        Shell_NotifyIconW(NIM_MODIFY, &data);
    } else {
        addIcon();
    }
}

void TrayIconWin::updateToolTip()
{
    if (!m_registered)
        return;

    auto data = notifyIconData(HWND(m_window->winId()), NIF_TIP | NIF_SHOWTIP);
    copyText(m_toolTip, data.szTip);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}
