#include "menuwidget.h"

#include <QActionEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QMenu>

MenuWidget::MenuWidget(QMenu *menu, QAction *action, QWidget *parent) :
    QWidget(parent), m_menu(menu), m_action(action)
{
    connect(this, &MenuWidget::layoutChanged, this, &MenuWidget::relayoutMenu);
}

bool MenuWidget::event(QEvent *event)
{
    const QEvent::Type type = event->type();
    const bool res = QWidget::event(event);

    switch (type) {
    case QEvent::ChildAdded:
    case QEvent::ChildRemoved:
    case QEvent::LayoutRequest: {
        emit layoutChanged();
    } break;
    default:
        break;
    }

    return res;
}

void MenuWidget::relayoutMenu()
{
    QActionEvent e(QEvent::ActionChanged, m_action);
    QCoreApplication::sendEvent(m_menu, &e);
}
