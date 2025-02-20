#ifndef TASKBARBUTTON_H
#define TASKBARBUTTON_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWindow)

struct ITaskbarList4;

class TaskbarButton final
{
public:
    explicit TaskbarButton();
    ~TaskbarButton();

    void setApplicationBadge(const QIcon &icon);
    void setWindowBadge(QWindow *window);

private:
    void setupTaskbarIface();
    void closeTaskbarIface();

    void setBadgeIcon(const QIcon &icon);
    void closeBadgeIcon();

private:
    using IconHandle = void *;

    ITaskbarList4 *m_taskbarIface = nullptr;
    IconHandle m_badgeIcon = nullptr;
};

#endif // TASKBARBUTTON_H
