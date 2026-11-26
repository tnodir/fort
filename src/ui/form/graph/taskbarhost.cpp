#include "taskbarhost.h"

#include <QLoggingCategory>
#include <QOperatingSystemVersion>
#include <QWidget>
#include <QtMath>

#include <qt_windows.h>

#include <algorithm>

namespace {

const QLoggingCategory LC("form.graph.taskbarHost");

inline constexpr wchar_t messageWindowClass[] = L"FortFirewallTaskbarHost";

UINT g_taskbarCreatedMsg = 0;

TaskbarHost *g_taskbarHost = nullptr; // for the static callbacks

// The layered child windows are since Windows 8
bool canLayerChildWindows()
{
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8;
}

HWND toHwnd(WId wid)
{
    return reinterpret_cast<HWND>(wid);
}

HWND windowHwnd(QWidget *window)
{
    return toHwnd(window->winId());
}

QRect windowRect(HWND hwnd, HWND parent = nullptr)
{
    RECT r;
    if (!hwnd || !GetWindowRect(hwnd, &r))
        return {};

    // In the parent's client coordinates, else in the screen's
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<POINT *>(&r), 2);

    return QRect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

// The widget's geometry follows the native one at once (exact in the device-independent pixels):
// else the native show after its show event applies the widget's own geometry
void syncWidgetGeometry(QWidget *window)
{
    window->setGeometry(window->windowHandle()->geometry());
}

bool isWindowVisible(HWND hwnd)
{
    return hwnd && IsWindowVisible(hwnd);
}

int physicalLength(QWidget *window, int length)
{
    return qRound(length * window->devicePixelRatioF());
}

// Qt keeps a window's geometry in the device-independent pixels: the native one is rounded to them
// and back, and a layered window is painted at the latter (QHighDpi::toNativePixels() of its frame
// geometry), e.g. a coordinate of 5n + 2 is moved by 1 at 125%
bool isNativeExact(int value, qreal dpr)
{
    const int logical = qRound(value * (qreal(1) / dpr)); // as QHighDpi::fromNativePixels()

    return qRound(logical * dpr) == value;
}

int nativeExactUp(int value, qreal dpr)
{
    while (!isNativeExact(value, dpr)) {
        ++value;
    }
    return value;
}

int nativeExactDown(int value, qreal dpr)
{
    while (!isNativeExact(value, dpr)) {
        --value;
    }
    return value;
}

struct Span
{
    int start = 0;
    int length = 0;

    int end() const { return start + length; }
};

bool isBetterSpan(const Span &span, const Span &best)
{
    // Up to the end: flush with the notification area and the next window, else the longest
    if (span.end() != best.end()) {
        return span.end() > best.end();
    }

    return span.length > best.length;
}

// Inside [start, end), of the native exact start and length
Span nativeExactSpan(int start, int end, qreal dpr)
{
    Span best = { nativeExactUp(start, dpr), 0 };

    const int maxStart = qMin(start + qCeil(dpr), end - 1);

    for (int s = best.start; s <= maxStart; s = nativeExactUp(s + 1, dpr)) {
        const Span span = { s, nativeExactDown(end - s, dpr) };

        if (isBetterSpan(span, best)) {
            best = span;
        }
    }

    return best;
}

// Of the span along the bar, across all of it
QRect barSpanRect(const QRect &bar, const Span &span, bool vertical, qreal dpr)
{
    if (vertical) {
        const Span cross = nativeExactSpan(bar.left(), bar.right() + 1, dpr);
        return QRect(cross.start, span.start, cross.length, span.length);
    }

    const Span cross = nativeExactSpan(bar.top(), bar.bottom() + 1, dpr);
    return QRect(span.start, cross.start, span.length, cross.length);
}

bool isVertical(const QRect &rect)
{
    return rect.height() > rect.width();
}

// The vertical task list's bottom edge, the notification area's near one, else the taskbar's far
// one
int endEdge(const QRect &bar, const QRect &end, bool vertical)
{
    if (end.isEmpty()) {
        return vertical ? bar.bottom() + 1 : bar.right() + 1;
    }

    return vertical ? end.bottom() + 1 : end.left();
}

}

struct TaskbarHostEvents
{
    static LRESULT CALLBACK messageWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == g_taskbarCreatedMsg) {
            // The message window is destroyed by the handler
            auto host = g_taskbarHost;
            QMetaObject::invokeMethod(
                    host, [host] { host->onTaskbarCreated(); }, Qt::QueuedConnection);
            return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static void CALLBACK winEventProc(HWINEVENTHOOK /*hook*/, DWORD /*event*/, HWND hwnd,
            LONG idObject, LONG idChild, DWORD /*thread*/, DWORD /*time*/)
    {
        if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
            return;

        g_taskbarHost->onTaskbarWindowChanged(WId(hwnd));
    }

    static HWND createMessageWindow()
    {
        static const ATOM atom = [] {
            WNDCLASSW wc = {};
            wc.lpfnWndProc = messageWindowProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = messageWindowClass;

            return RegisterClassW(&wc);
        }();

        if (!atom)
            return nullptr;

        // Not a message-only window: it gets the broadcast messages
        return CreateWindowExW(0, messageWindowClass, nullptr, WS_POPUP, 0, 0, 0, 0, nullptr,
                nullptr, GetModuleHandleW(nullptr), nullptr);
    }

    static void *setWinEventHook(DWORD event, DWORD processId, DWORD threadId)
    {
        return SetWinEventHook(
                event, event, nullptr, winEventProc, processId, threadId, WINEVENT_OUTOFCONTEXT);
    }
};

TaskbarHost::TaskbarHost(QObject *parent) : QObject(parent)
{
    g_taskbarHost = this;
}

TaskbarHost::~TaskbarHost()
{
    removeAllWindows();

    g_taskbarHost = nullptr;
}

bool TaskbarHost::canLayerWindows() const
{
    // The mode of the placed windows, else of the taskbar's layout
    const Mode mode = m_windows.isEmpty() ? taskbarMode(findTaskbar()) : m_taskbarMode;

    return isLayeredMode(mode);
}

void TaskbarHost::addWindow(QWidget *window, const WindowOptions &options)
{
    // Destroyed with the old taskbar: waits for its recreation
    if (!IsWindow(windowHwnd(window)))
        return;

    if (m_windows.isEmpty()) {
        attach();
    }

    int index = windowIndex(window);
    if (index < 0) {
        index = insertIndex(options.order);
        m_windows.insert(index, { window, options });

        embedWindow(window);
    } else {
        m_windows[index].options = options;
    }

    layout();
}

void TaskbarHost::removeWindow(QWidget *window)
{
    const int index = windowIndex(window);
    if (index < 0)
        return;

    m_windows.removeAt(index);

    unembedWindow(window);

    layout(); // the task list gets its width back

    if (m_windows.isEmpty()) {
        detach();
    }
}

void TaskbarHost::removeAllWindows()
{
    unembedWindows();

    m_windows.clear();

    detach();
}

void TaskbarHost::invalidateWindows()
{
    removeAllWindows();

    emit windowsInvalidated();
}

void TaskbarHost::attach()
{
    m_bar = findTaskbar();
    if (!m_bar.taskbar) {
        qCWarning(LC) << "Taskbar not found";
    }

    m_taskbarMode = m_mode = taskbarMode(m_bar);

    startEvents();
}

void TaskbarHost::detach()
{
    stopEvents();

    releaseParentWindow();

    m_taskbarMode = m_mode = ModeNone;
    m_shrunkWidth = 0;
    m_shrunkTaskList = {};

    m_bar = {};
}

TaskbarHost::TaskbarWindows TaskbarHost::findTaskbar()
{
    const HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!taskbar)
        return {};

    const HWND rebar = FindWindowExW(taskbar, nullptr, L"ReBarWindow32", nullptr);

    TaskbarWindows bar;
    bar.taskbar = WId(taskbar);
    bar.rebar = WId(rebar);
    bar.taskList = rebar ? WId(FindWindowExW(rebar, nullptr, L"MSTaskSwWClass", nullptr)) : 0;
    bar.notify = WId(FindWindowExW(taskbar, nullptr, L"TrayNotifyWnd", nullptr));

    return bar;
}

TaskbarHost::Mode TaskbarHost::taskbarMode(const TaskbarWindows &bar)
{
    const QRect rect = windowRect(toHwnd(bar.taskbar));
    if (rect.isEmpty())
        return ModeNone;

    // The vertical taskbar isn't embedded into
    if (isVertical(rect))
        return ModeOverlay;

    // Windows 11 keeps the old task list's window, empty, with its list hidden
    const HWND taskList = bar.taskList
            ? FindWindowExW(toHwnd(bar.taskList), nullptr, L"MSTaskListWClass", nullptr)
            : nullptr;

    return isWindowVisible(taskList) ? ModeTaskList : ModeTaskbar;
}

bool TaskbarHost::isLayeredMode(Mode mode)
{
    // The overlay is a top-level window
    return mode == ModeOverlay || canLayerChildWindows();
}

void TaskbarHost::updateMode()
{
    // The taskbar is moved to another edge
    const Mode mode = taskbarMode(m_bar);
    if (mode == m_taskbarMode || mode == ModeNone)
        return;

    if (isLayeredMode(mode) != isLayeredMode(m_taskbarMode)) {
        invalidateWindows();
        return;
    }

    unembedWindows();

    m_taskbarMode = m_mode = mode;
    m_shrunkWidth = 0;
    m_shrunkTaskList = {};

    embedWindows();
}

WId TaskbarHost::parentHandle() const
{
    switch (m_mode) {
    case ModeTaskList:
        return m_bar.rebar;
    case ModeTaskbar:
        return m_bar.taskbar;
    default:
        return 0;
    }
}

void TaskbarHost::startEvents()
{
    if (!g_taskbarCreatedMsg) {
        g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");
    }

    const HWND messageWindow = TaskbarHostEvents::createMessageWindow();
    m_messageWindow = WId(messageWindow);

    // From the Explorer to the elevated process
    if (!messageWindow
            || !ChangeWindowMessageFilterEx(
                    messageWindow, g_taskbarCreatedMsg, MSGFLT_ALLOW, nullptr)) {
        qCWarning(LC) << "TaskbarCreated message error:" << GetLastError();
    }

    if (!m_bar.taskbar)
        return;

    // The taskbar's windows are of one thread
    DWORD processId = 0;
    const DWORD threadId = GetWindowThreadProcessId(toHwnd(m_bar.taskbar), &processId);

    m_locationHook =
            TaskbarHostEvents::setWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, processId, threadId);
    m_foregroundHook =
            TaskbarHostEvents::setWinEventHook(EVENT_SYSTEM_FOREGROUND, processId, threadId);

    if (!m_locationHook || !m_foregroundHook) {
        qCWarning(LC) << "WinEvent hook error:" << GetLastError();
    }
}

void TaskbarHost::stopEvents()
{
    for (void **hook : { &m_locationHook, &m_foregroundHook }) {
        if (*hook) {
            UnhookWinEvent(HWINEVENTHOOK(*hook));
            *hook = nullptr;
        }
    }

    if (m_messageWindow) {
        DestroyWindow(toHwnd(m_messageWindow));
        m_messageWindow = 0;
    }
}

void TaskbarHost::embedWindows()
{
    for (const auto &hosted : std::as_const(m_windows)) {
        embedWindow(hosted.window);
    }
}

void TaskbarHost::unembedWindows()
{
    for (const auto &hosted : std::as_const(m_windows)) {
        unembedWindow(hosted.window);
    }

    releaseParentWindow();
}

void TaskbarHost::embedWindow(QWidget *window)
{
    const WId parent = parentHandle();
    if (!parent)
        return; // the overlay

    if (!m_parentWindow) {
        m_parentWindow = QWindow::fromWinId(parent);
    }

    window->winId(); // creates the window's handle
    window->windowHandle()->setParent(m_parentWindow);

    if (GetParent(windowHwnd(window)) != toHwnd(parent)) {
        qCWarning(LC) << "Embedding error: the overlay is used";
        useOverlay();
    }
}

void TaskbarHost::unembedWindow(QWidget *window)
{
    QWindow *windowHandle = window->windowHandle();

    if (windowHandle && windowHandle->parent()) {
        windowHandle->setParent(nullptr);
    }
}

void TaskbarHost::useOverlay()
{
    unembedWindows();

    m_mode = ModeOverlay;
}

void TaskbarHost::releaseParentWindow()
{
    // Without the child windows: the foreign window would delete them
    delete m_parentWindow;
    m_parentWindow = nullptr;
}

int TaskbarHost::windowIndex(QWidget *window) const
{
    const auto it = std::find_if(m_windows.cbegin(), m_windows.cend(),
            [&](const HostedWindow &hosted) { return hosted.window == window; });

    return (it != m_windows.cend()) ? int(it - m_windows.cbegin()) : -1;
}

int TaskbarHost::insertIndex(int order) const
{
    // After the windows of the same order
    const auto it = std::upper_bound(m_windows.cbegin(), m_windows.cend(), order,
            [](int order, const HostedWindow &hosted) { return order < hosted.options.order; });

    return int(it - m_windows.cbegin());
}

int TaskbarHost::totalLength() const
{
    int length = 0;

    for (const auto &hosted : m_windows) {
        length += physicalLength(hosted.window, hosted.options.length);
    }

    return length;
}

void TaskbarHost::layout()
{
    switch (m_mode) {
    case ModeTaskList: {
        layoutTaskList();
    } break;
    case ModeTaskbar: {
        layoutBeforeNotify(m_bar.taskbar, WId(HWND_TOP));
    } break;
    case ModeOverlay: {
        layoutBeforeNotify(/*parent=*/0, WId(HWND_TOPMOST));
    } break;
    default:
        break;
    }
}

void TaskbarHost::layoutTaskList()
{
    const HWND taskList = toHwnd(m_bar.taskList);

    const QRect rect = windowRect(taskList, toHwnd(m_bar.rebar));
    if (rect.isEmpty())
        return;

    // The full width is of the Explorer's layout, else of ours
    const int fullWidth = rect.width() + (rect == m_shrunkTaskList ? m_shrunkWidth : 0);
    const int width = qMax(fullWidth - totalLength(), 0);

    m_shrunkTaskList = QRect(rect.x(), rect.y(), width, rect.height());
    m_shrunkWidth = fullWidth - width;

    if (m_shrunkTaskList != rect) {
        // Not waiting for the Explorer's thread
        SetWindowPos(taskList, nullptr, rect.x(), rect.y(), width, rect.height(),
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
    }

    placeWindows(rect, rect.x() + fullWidth, /*vertical=*/false, WId(HWND_TOP));
}

void TaskbarHost::layoutBeforeNotify(WId parent, WId insertAfter)
{
    const QRect rect = windowRect(toHwnd(m_bar.taskbar), toHwnd(parent));
    if (rect.isEmpty())
        return;

    // The vertical taskbar's notification area is at its bottom, after the bands of the task list
    // and e.g. the language bar
    const bool vertical = isVertical(rect);

    const WId endWindow = vertical ? m_bar.taskList : m_bar.notify;
    const QRect endRect = windowRect(toHwnd(endWindow), toHwnd(parent));
    const int end = endEdge(rect, endRect, vertical);

    placeWindows(rect, end, vertical, insertAfter);
}

void TaskbarHost::placeWindows(const QRect &bar, int end, bool vertical, WId insertAfter)
{
    // From the end: the first window is the nearest to the notification area
    for (const auto &hosted : std::as_const(m_windows)) {
        QWidget *window = hosted.window;
        const qreal dpr = window->devicePixelRatioF();

        const int length = physicalLength(window, hosted.options.length);
        const Span span = nativeExactSpan(end - length, end, dpr);
        end = span.start;

        const QRect rect = barSpanRect(bar, span, vertical, dpr);

        SetWindowPos(windowHwnd(window), toHwnd(insertAfter), rect.x(), rect.y(), rect.width(),
                rect.height(), SWP_NOACTIVATE);

        syncWidgetGeometry(window);
    }
}

bool TaskbarHost::isTaskbarWindow(WId hwnd) const
{
    if (!hwnd)
        return false;

    const WId windows[] = { m_bar.taskbar, m_bar.rebar, m_bar.taskList, m_bar.notify };

    return std::find(std::cbegin(windows), std::cend(windows), hwnd) != std::cend(windows);
}

void TaskbarHost::onTaskbarWindowChanged(WId hwnd)
{
    // The taskbar's layout is changed or it's over the overlay
    if (isTaskbarWindow(hwnd)) {
        updateMode();
        layout();
    }
}

void TaskbarHost::onTaskbarCreated()
{
    if (m_windows.isEmpty())
        return;

    // The embedded windows are destroyed with the old taskbar
    invalidateWindows();
}
