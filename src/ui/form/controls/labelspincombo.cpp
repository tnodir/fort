#include "labelspincombo.h"

#include <QHBoxLayout>
#include <QLabel>

LabelSpinCombo::LabelSpinCombo(QWidget *parent) :
    SpinCombo(parent)
{
    setupUi();
}

void LabelSpinCombo::setupUi()
{
    m_label = new QLabel();

    boxLayout()->insertWidget(0, m_label, 1);
}
