#include "widebutton.h"

WideButton::WideButton(QWidget *parent) :
    QPushButton(parent)
{
}

WideButton::WideButton(const QIcon &icon, const QString &text, QWidget *parent) :
    QPushButton(icon, text, parent)
{
}

QSize WideButton::minimumSizeHint() const
{
    auto size = QPushButton::minimumSizeHint();
    size.setWidth(size.width() + 3);
    return size;
}
