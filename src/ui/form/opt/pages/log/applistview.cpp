#include "applistview.h"

AppListView::AppListView(QWidget *parent) :
    QListView(parent)
{
    setupUi();
}

void AppListView::setupUi()
{
    this->setFlow(QListView::TopToBottom);
    this->setViewMode(QListView::ListMode);
    this->setItemAlignment(Qt::AlignLeft);
    this->setIconSize(QSize(24, 24));
    this->setUniformItemSizes(true);
    this->setAlternatingRowColors(true);
}
