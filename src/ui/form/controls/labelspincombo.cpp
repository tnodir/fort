#include "labelspincombo.h"

#include <QHBoxLayout>
#include <QLabel>

#include "controlutil.h"

LabelSpinCombo::LabelSpinCombo(QWidget *parent) : SpinCombo(parent)
{
    setupUi();
}

void LabelSpinCombo::setupUi()
{
    m_label = ControlUtil::createLabel();

    boxLayout()->insertWidget(0, m_label, 1);
}
