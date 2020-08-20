#include "tabbar.h"

TabBar::TabBar(QWidget *parent) : QTabBar(parent) { }

QSize TabBar::tabSizeHint(int index) const
{
    auto size = QTabBar::tabSizeHint(index);
    if (tabMinimumWidth() != 0) {
        size.setWidth(qMax(size.width(), tabMinimumWidth()));
    }
    return size;
}
