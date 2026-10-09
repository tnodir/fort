#include "widgetwindow.h"

#include <QGuiApplication>
#include <QWindowStateChangeEvent>

#include <util/osutil.h>

namespace {

bool isInRange(int pos, int length, int min, int max)
{
    return pos >= min && pos + length - 1 <= max;
}

/* Below the rect or above it, if only that fits the screen */
int nextToY(const QRect &r, int height, const QRect &screen, int spacing)
{
    const int belowY = r.bottom() + 1 + spacing;
    if (isInRange(belowY, height, screen.top(), screen.bottom()))
        return belowY;

    const int aboveY = r.top() - spacing - height;
    if (isInRange(aboveY, height, screen.top(), screen.bottom()))
        return aboveY;

    return belowY;
}

/* By the rect's left edge or by its right edge, if only that fits the screen */
int nextToX(const QRect &r, int width, const QRect &screen)
{
    const int leftX = r.left();
    if (isInRange(leftX, width, screen.left(), screen.right()))
        return leftX;

    const int rightX = r.right() + 1 - width;
    if (isInRange(rightX, width, screen.left(), screen.right()))
        return rightX;

    return leftX;
}

}

WidgetWindow::WidgetWindow(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f) { }

void WidgetWindow::showWindow(bool activate)
{
    if (isHidden()) {
        emit aboutToShow();
    }

    showWidget(this, activate);
}

void WidgetWindow::exposeWindow()
{
    exposeWidget(this);
}

void WidgetWindow::centerTo(QWidget *w)
{
    this->move(w->frameGeometry().topLeft() + w->rect().center() - this->rect().center());
}

void WidgetWindow::centerTo(QScreen *s)
{
    const QRect r = s->availableGeometry();

    this->move(r.center() - this->rect().center());
}

void WidgetWindow::moveNextTo(QWidget *w, int spacing)
{
    const QRect r = w->frameGeometry();
    const QRect screen = w->screen()->availableGeometry();

    // Not shown yet, without the frame: as the window's one
    const QSize frameSize = this->size() + (r.size() - w->size());

    this->move(
            nextToX(r, frameSize.width(), screen), nextToY(r, frameSize.height(), screen, spacing));
}

void WidgetWindow::showWidget(QWidget *w, bool activate)
{
    if (w->isMinimized()) {
        w->setWindowState(w->windowState() ^ Qt::WindowMinimized);
    }

    w->show();
    w->raise();

    if (activate) {
        w->activateWindow();
    }
}

void WidgetWindow::exposeWidget(QWidget *w)
{
    const auto flags = w->windowFlags();
    if ((flags & Qt::WindowStaysOnTopHint) != 0)
        return;

    w->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    w->show();

    w->setWindowFlags(flags);
    w->show();

    w->activateWindow();
}

void WidgetWindow::excludeWindowFromCapture(QWidget *w, bool exclude)
{
    OsUtil::excludeWindowFromCapture(w, exclude);
}

void WidgetWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);

    emit positionChanged();
}

void WidgetWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    emit sizeChanged();
}

void WidgetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    emit visibilityChanged(/*isVisible=*/true);
}

void WidgetWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    emit visibilityChanged(/*isVisible=*/false);
}

void WidgetWindow::closeEvent(QCloseEvent *event)
{
    if (checkAboutToClose()) {
        event->accept();

        emit aboutToClose();
    } else {
        event->ignore();
    }
}

void WidgetWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);

    if (event->modifiers() != Qt::NoModifier)
        return;

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        emit defaultKeyPressed();
    } break;
    case Qt::Key_Escape: {
        close();
    } break;
    }
}

void WidgetWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::WindowStateChange: {
        auto e = static_cast<QWindowStateChangeEvent *>(event);

        const Qt::WindowStates newState = windowState();
        if (newState != e->oldState()) {
            emit visibilityChanged(/*isVisible=*/(newState != Qt::WindowNoState));
        }
    } break;
    default:
        break;
    }
}

bool WidgetWindow::event(QEvent *event)
{
    const QEvent::Type type = event->type();
    const bool res = QWidget::event(event);

    switch (type) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
        const bool isActive = (type == QEvent::WindowActivate);
        emit activationChanged(isActive);
    } break;
    default:
        break;
    }

    return res;
}

void WidgetWindow::ensureWindowScreenBounds()
{
    const auto p = this->geometry().center();
    const auto screen = QGuiApplication::screenAt(p);

    if (!screen) {
        centerTo(QGuiApplication::primaryScreen());
    }
}
