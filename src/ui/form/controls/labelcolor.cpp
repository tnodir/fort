#include "labelcolor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <form/dialog/dialogutil.h>

#include "controlutil.h"

namespace {

QPushButton *createColorButton(const std::function<void()> &onClicked)
{
    auto c = new QPushButton();
    c->setFixedSize(40, 30);

    c->connect(c, &QPushButton::clicked, onClicked);

    return c;
}

void setButtonColor(QPushButton *c, const QColor &color)
{
    const auto qss = QString("background-color: %1").arg(color.name(QColor::HexArgb));
    c->setStyleSheet(qss);
}

}

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
    setupButtons();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_label, /*stretch*/ nullptr, m_button, m_darkButton }, /*margin=*/0);

    this->setLayout(layout);
}

void LabelColor::setupButtons()
{
    // Light
    m_button = createColorButton([&] { selectColor(); });

    connect(this, &LabelColor::colorChanged,
            [&] { setButtonColor(button(), color().name(QColor::HexArgb)); });

    // Dark
    m_darkButton = createColorButton([&] { selectDarkColor(); });

    connect(this, &LabelColor::darkColorChanged,
            [&] { setButtonColor(darkButton(), darkColor().name(QColor::HexArgb)); });
}
