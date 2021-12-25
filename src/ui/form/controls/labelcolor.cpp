#include "labelcolor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <form/dialog/dialogutil.h>

#include "controlutil.h"

LabelColor::LabelColor(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void LabelColor::setColor(const QColor &v)
{
    if (m_color != v) {
        m_color = v;
        emit colorChanged(m_color);
    }
}

void LabelColor::selectColor()
{
    const auto title = tr("Select color for %1").arg(label()->text());
    const auto selectedColor = DialogUtil::getColor(color(), title);
    if (selectedColor.isValid()) {
        setColor(selectedColor);
    }
}

void LabelColor::setupUi()
{
    m_label = ControlUtil::createLabel();

    setupButton();

    auto layout = ControlUtil::createRowLayout(m_label, m_button);

    this->setLayout(layout);
}

void LabelColor::setupButton()
{
    m_button = new QPushButton();
    m_button->setFixedSize(40, 30);

    connect(button(), &QPushButton::clicked, this, &LabelColor::selectColor);

    connect(this, &LabelColor::colorChanged, [&] {
        const auto qss = QString("background-color: %1").arg(color().name());
        button()->setStyleSheet(qss);
    });
}
