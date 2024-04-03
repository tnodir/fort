#include "spincombo.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QSpinBox>

#include "controlutil.h"

SpinCombo::SpinCombo(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void SpinCombo::setValues(const ValuesList &v)
{
    if (m_values != v) {
        m_values = v;
        emit valuesChanged();
    }
}

void SpinCombo::setNames(const QStringList &v)
{
    if (m_names != v) {
        m_names = v;
        emit namesChanged();
    }
}

void SpinCombo::setNamesByValues()
{
    QStringList list;
    for (int v : values()) {
        list.append(QString::number(v));
    }
    setNames(list);
}

QHBoxLayout *SpinCombo::boxLayout() const
{
    return static_cast<QHBoxLayout *>(layout());
}

void SpinCombo::setupUi()
{
    setupSpin();
    setupCombo();

    auto layout = ControlUtil::createRowLayout(m_spinBox, m_comboBox, /*stretch1=*/0);

    this->setLayout(layout);
}

void SpinCombo::setupSpin()
{
    m_spinBox = ControlUtil::createSpinBox();
    m_spinBox->setFixedWidth(110);
    m_spinBox->setRange(0, 9999);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &SpinCombo::updateComboBoxIndex);
}

void SpinCombo::setupCombo()
{
    m_comboBox = ControlUtil::createComboBox();
    m_comboBox->setFixedWidth(110);

    connect(m_comboBox, QOverload<int>::of(&QComboBox::activated), this,
            &SpinCombo::updateSpinBoxValue);

    connect(this, &SpinCombo::namesChanged, [&] {
        ControlUtil::setComboBoxTexts(m_comboBox, names());

        updateComboBoxIndex(m_spinBox->value());
    });
}

void SpinCombo::updateSpinBoxValue(int index)
{
    m_spinBox->setValue(values().at(index));
}

void SpinCombo::updateComboBoxIndex(int value)
{
    m_comboBox->setCurrentIndex(getIndexByValue(value));
}

int SpinCombo::getIndexByValue(int value) const
{
    const int index = values().indexOf(value);
    return (index <= 0) ? 0 : index;
}
