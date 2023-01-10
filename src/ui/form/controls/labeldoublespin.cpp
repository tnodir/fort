#include "labeldoublespin.h"

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>

#include "controlutil.h"

LabelDoubleSpin::LabelDoubleSpin(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

QHBoxLayout *LabelDoubleSpin::boxLayout() const
{
    return static_cast<QHBoxLayout *>(layout());
}

void LabelDoubleSpin::setupUi()
{
    m_label = ControlUtil::createLabel();

    setupSpin();

    auto layout = ControlUtil::createRowLayout(m_label, m_spinBox);

    this->setLayout(layout);
}

void LabelDoubleSpin::setupSpin()
{
    m_spinBox = new QDoubleSpinBox();
    m_spinBox->setMinimumWidth(110);
    m_spinBox->setRange(0, 9999);
}
