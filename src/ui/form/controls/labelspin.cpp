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
    m_label = ControlUtil::createLabel();

    setupSpin();

    auto layout = ControlUtil::createRowLayout(m_label, m_spinBox);

    this->setLayout(layout);
}

void LabelSpin::setupSpin()
{
    m_spinBox = ControlUtil::createSpinBox();
    m_spinBox->setFixedWidth(110);
    m_spinBox->setRange(0, 9999);
}
