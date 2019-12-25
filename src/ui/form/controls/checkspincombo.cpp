#include "checkspincombo.h"

#include <QCheckBox>
#include <QHBoxLayout>

CheckSpinCombo::CheckSpinCombo(QWidget *parent) :
    SpinCombo(parent)
{
    setupUi();
}

void CheckSpinCombo::setupUi()
{
    m_checkBox = new QCheckBox();

    boxLayout()->insertWidget(0, m_checkBox, 1);
}
