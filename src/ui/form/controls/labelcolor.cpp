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

void LabelColor::setDarkColor(const QColor &v)
{
    if (m_darkColor != v) {
        m_darkColor = v;
        emit darkColorChanged(m_darkColor);
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

void LabelColor::selectDarkColor()
{
    const auto title = tr("Select dark color for %1").arg(label()->text());
    const auto selectedColor = DialogUtil::getColor(darkColor(), title);
    if (selectedColor.isValid()) {
        setDarkColor(selectedColor);
    }
}

void LabelColor::setupUi()
{
    m_label = ControlUtil::createLabel();

    // Color Buttons
    setupButton();
    setupDarkButton();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_label, /*stretch*/ nullptr, m_button, m_darkButton }, /*margin=*/0);

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

void LabelColor::setupDarkButton()
{
    m_darkButton = new QPushButton();
    m_darkButton->setFixedSize(40, 30);

    connect(darkButton(), &QPushButton::clicked, this, &LabelColor::selectDarkColor);

    connect(this, &LabelColor::darkColorChanged, [&] {
        const auto qss = QString("background-color: %1").arg(darkColor().name());
        darkButton()->setStyleSheet(qss);
    });
}
