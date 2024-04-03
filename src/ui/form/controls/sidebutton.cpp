#include "sidebutton.h"

#include <QStylePainter>

namespace {

constexpr int indicatorWidth = 4;

}

SideButton::SideButton(QWidget *parent) : QToolButton(parent) { }

QSize SideButton::sizeHint() const
{
    QSize sh = QToolButton::sizeHint();
    sh.setWidth(sh.width() + indicatorWidth);
    return sh;
}

void SideButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);

    if (!isChecked())
        return;

    QRect r = rect();
    if (isRightToLeft()) {
        r.setX(r.width() - indicatorWidth);
    }
    r.setWidth(indicatorWidth);

    QPainter p(this);
    p.fillRect(r, QColor(0x3a, 0xd2, 0x4c));
}
