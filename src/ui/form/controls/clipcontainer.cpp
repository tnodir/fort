#include "clipcontainer.h"

#include <QEvent>

ClipContainer::ClipContainer(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ClipContainer::setWidget(QWidget *w)
{
    m_widget = w;

    if (w) {
        w->setParent(this);
        w->show();
    }

    updateGeometry();
    updateWidgetGeometry();
}

QWidget *ClipContainer::takeWidget()
{
    auto w = m_widget;

    m_widget = nullptr;

    updateGeometry();

    return w;
}

QSize ClipContainer::sizeHint() const
{
    return m_widget ? m_widget->sizeHint() : QSize();
}

QSize ClipContainer::minimumSizeHint() const
{
    return m_widget ? QSize(0, m_widget->minimumSizeHint().height()) : QSize(0, 0);
}

bool ClipContainer::event(QEvent *event)
{
    // The widget's size hints changed
    if (event->type() == QEvent::LayoutRequest) {
        updateGeometry();
        updateWidgetGeometry();
    }

    return QWidget::event(event);
}

void ClipContainer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateWidgetGeometry();
}

void ClipContainer::updateWidgetGeometry()
{
    if (!m_widget)
        return;

    const int width = qMax(this->width(), m_widget->minimumSizeHint().width());

    m_widget->setGeometry(0, 0, width, height());
}
