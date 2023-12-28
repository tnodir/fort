#include "labeldoublespin.h"

#include <QHBoxLayout>
#include <QLabel>

#include "controlutil.h"
#include "doublespinbox.h"

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
    m_spinBox = new DoubleSpinBox();
    m_spinBox->setFixedWidth(110);
    m_spinBox->setRange(0, 9999);
}
