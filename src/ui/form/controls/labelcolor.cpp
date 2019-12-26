#include "labelcolor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "controlutil.h"

LabelColor::LabelColor(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void LabelColor::setColor(const QColor &v)
{
    if (m_color != v) {
        m_color = v;
        emit colorChanged();
    }
}

void LabelColor::selectColor()
{
    const auto title = tr("Select color for %1")
            .arg(label()->text());
    const auto selectedColor = ControlUtil::getColor(color(), title);
    if (selectedColor.isValid()) {
        setColor(selectedColor);
    }
}

void LabelColor::setupUi()
{
    auto layout = new QHBoxLayout();
    layout->setMargin(0);

    m_label = new QLabel();

    setupButton();

    layout->addWidget(m_label, 1);
    layout->addWidget(m_button);

    this->setLayout(layout);
}

void LabelColor::setupButton()
{
    m_button = new QPushButton();
    m_button->setFixedSize(40, 30);

    connect(button(), &QPushButton::clicked, this, &LabelColor::selectColor);

    connect(this, &LabelColor::colorChanged, [&] {
        const auto qss = QString("background-color: %1")
                .arg(color().name());
        button()->setStyleSheet(qss);
    });
}
