#include "labelspin.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include "controlutil.h"

LabelSpin::LabelSpin(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

QHBoxLayout *LabelSpin::boxLayout() const
{
    return static_cast<QHBoxLayout *>(layout());
}

void LabelSpin::setupUi()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = ControlUtil::createLabel();

    setupSpin();

    layout->addWidget(m_label, 1);
    layout->addWidget(m_spinBox);

    this->setLayout(layout);
}

void LabelSpin::setupSpin()
{
    m_spinBox = new QSpinBox();
    m_spinBox->setMinimumWidth(110);
    m_spinBox->setRange(0, 9999);
}
