#include "checkspincombo.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSpinBox>

CheckSpinCombo::CheckSpinCombo(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void CheckSpinCombo::setValues(const ValuesList &v)
{
    if (m_values != v) {
        m_values = v;
        emit valuesChanged();
    }
}

void CheckSpinCombo::setNames(const QStringList &v)
{
    if (m_names != v) {
        m_names = v;
        emit namesChanged();
    }
}

void CheckSpinCombo::setupUi()
{
    auto layout = new QHBoxLayout();

    m_checkBox = new QCheckBox();

    setupSpin();
    setupCombo();

    layout->addWidget(m_checkBox, 1);
    layout->addWidget(m_spinBox);
    layout->addWidget(m_comboBox);

    this->setLayout(layout);
}

void CheckSpinCombo::setupSpin()
{
    m_spinBox = new QSpinBox();
    m_spinBox->setMinimumWidth(110);
    m_spinBox->setRange(0, 9999);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CheckSpinCombo::updateComboBoxIndex);
}

void CheckSpinCombo::setupCombo()
{
    m_comboBox = new QComboBox();

    connect(m_comboBox, QOverload<int>::of(&QComboBox::activated),
            this, &CheckSpinCombo::updateSpinBoxValue);

    connect(this, &CheckSpinCombo::namesChanged, [&] {
        m_comboBox->clear();
        m_comboBox->addItems(names());

        updateComboBoxIndex(m_spinBox->value());
    });
}

void CheckSpinCombo::updateSpinBoxValue(int index)
{
    m_spinBox->setValue(values().at(index));
}

void CheckSpinCombo::updateComboBoxIndex(int value)
{
    m_comboBox->setCurrentIndex(getIndexByValue(value));
}

int CheckSpinCombo::getIndexByValue(int value) const
{
    const int index = values().indexOf(value);
    return (index <= 0) ? 0 : index;
}
