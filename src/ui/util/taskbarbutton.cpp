#include "taskbarbutton.h"

#include <QGuiApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QWidget>
#include <QWindow>

#define WIN32_LEAN_AND_MEAN
#include <qt_windows.h>

#include "ShObjIdl_core.h"

namespace {

const QLoggingCategory LC("util.taskbarButton");

}

TaskbarButton::TaskbarButton()
{
    setupTaskbarIface();
}

TaskbarButton::~TaskbarButton()
{
    closeTaskbarIface();
    closeBadgeIcon();
}

void TaskbarButton::setupTaskbarIface()
{
    HRESULT hresult = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
            IID_ITaskbarList4, (void **) &m_taskbarIface);

    if (FAILED(hresult)) {
        qCWarning(LC) << "ITaskbarList4 create error:" << hresult;
        return;
    }

    hresult = m_taskbarIface->HrInit();

    if (FAILED(hresult)) {
        closeTaskbarIface();

        qCWarning(LC) << "ITaskbarList4 initialize error:" << hresult;
    }
}

void TaskbarButton::closeTaskbarIface()
{
    if (m_taskbarIface) {
        m_taskbarIface->Release();
        m_taskbarIface = nullptr;
    }
}

void TaskbarButton::setBadgeIcon(const QIcon &icon)
{
    closeBadgeIcon();

    if (icon.isNull())
        return;

    const QSize iconSize(16, 16);
    const qreal dpr = qApp->devicePixelRatio();

    const QImage image = icon.pixmap(iconSize, dpr).toImage();

    m_badgeIcon = image.toHICON();
}

void TaskbarButton::closeBadgeIcon()
{
    if (m_badgeIcon) {
        DestroyIcon(HICON(m_badgeIcon));
        m_badgeIcon = nullptr;
    }
}

void TaskbarButton::setApplicationBadge(const QIcon &icon)
{
    setBadgeIcon(icon);

    const auto topLevelWindows = QGuiApplication::topLevelWindows();
    for (QWindow *window : topLevelWindows) {
        if (window->handle()) {
            setWindowBadge(window);
        }
    }
}

void TaskbarButton::setWindowBadge(QWindow *window)
{
    if (Q_UNLIKELY(!m_taskbarIface))
        return;

    if (!window || !window->isVisible())
        return;

    m_taskbarIface->SetOverlayIcon(HWND(window->winId()), HICON(m_badgeIcon), L"");
}
