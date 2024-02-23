#include "checkspincombo.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>

CheckSpinCombo::CheckSpinCombo(QWidget *parent) : SpinCombo(parent)
{
    setupUi();
}

void CheckSpinCombo::setupUi()
{
    m_checkBox = new QCheckBox();

    boxLayout()->insertWidget(0, m_checkBox, 1);

    connect(comboBox(), QOverload<int>::of(&QComboBox::activated), this, [&](int index) {
        if (disabledIndex() != -1) {
            checkBox()->setChecked(index != disabledIndex());
        }
    });
}
