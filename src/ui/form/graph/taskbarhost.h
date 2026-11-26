#ifndef TASKBARHOST_H
#define TASKBARHOST_H

#include <QList>
#include <QObject>
#include <QWindow>

QT_FORWARD_DECLARE_CLASS(QWidget)

// Places the windows into the primary monitor's taskbar, before its notification area: left of
// it, at the task list's bottom on the vertical one; the nearest to it are of the lowest order
class TaskbarHost : public QObject
{
    Q_OBJECT

public:
    enum Mode : qint8 {
        ModeNone = 0, // no taskbar
        ModeTaskList, // Windows 7/10: in the room made by shrinking the task list
        ModeTaskbar, // Windows 11: in the taskbar, over its content
        ModeOverlay, // topmost over the taskbar: the vertical one or the embedding failed
    };

    struct WindowOptions
    {
        int order = 0; // the place from the notification area
        int length = 0; // along the taskbar, at 100% scale, px
    };

    explicit TaskbarHost(QObject *parent = nullptr);
    ~TaskbarHost() override;

    // The windows to place are created layered: since Windows 8, else for the overlay only
    bool canLayerWindows() const;

    // Places the window or updates its options
    void addWindow(QWidget *window, const WindowOptions &options);
    void removeWindow(QWidget *window);

signals:
    // The windows are to be created anew: the ones embedded in the old taskbar are destroyed with
    // it on the Explorer's restart, or the new mode needs another layering
    void windowsInvalidated();

private:
    struct TaskbarWindows
    {
        WId taskbar = 0;
        WId rebar = 0;
        WId taskList = 0;
        WId notify = 0;
    };

    struct HostedWindow
    {
        QWidget *window = nullptr;
        WindowOptions options;
    };

    void removeAllWindows();
    void invalidateWindows();

    void attach();
    void detach();

    static TaskbarWindows findTaskbar();
    static Mode taskbarMode(const TaskbarWindows &bar);
    static bool isLayeredMode(Mode mode);
    void updateMode();
    WId parentHandle() const;

    void startEvents();
    void stopEvents();

    void embedWindows();
    void unembedWindows();
    void embedWindow(QWidget *window);
    void unembedWindow(QWidget *window);
    void useOverlay();
    void releaseParentWindow();

    int windowIndex(QWidget *window) const;
    int insertIndex(int order) const;
    int totalLength() const;

    void layout();
    void layoutTaskList();
    void layoutBeforeNotify(WId parent, WId insertAfter);
    void placeWindows(const QRect &bar, int end, bool vertical, WId insertAfter);

    bool isTaskbarAboveWindows() const;
    bool isTaskbarWindow(WId hwnd) const;

    void onTaskbarWindowChanged(WId hwnd);
    void onZOrderChanged();
    void onTaskbarCreated();

    friend struct TaskbarHostEvents;

private:
    Mode m_mode = ModeNone;
    Mode m_taskbarMode = ModeNone; // of the taskbar's layout, m_mode is the overlay on an error

    int m_shrunkWidth = 0; // the task list's width given to the windows

    WId m_messageWindow = 0;

    void *m_locationHook = nullptr;
    void *m_foregroundHook = nullptr;
    void *m_reorderHook = nullptr;

    QWindow *m_parentWindow = nullptr; // the foreign taskbar's window

    TaskbarWindows m_bar;

    QRect m_shrunkTaskList; // set by the layout, in the rebar's coordinates

    QList<HostedWindow> m_windows;
};

#endif // TASKBARHOST_H
