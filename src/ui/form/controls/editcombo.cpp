#include "editcombo.h"

#include <QComboBox>
#include <QHBoxLayout>

#include "controlutil.h"
#include "lineedit.h"

EditCombo::EditCombo(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void EditCombo::setItems(const ValuesList &values, const QStringList &names)
{
    if (m_values == values && m_names == names)
        return;

    m_values = values;
    m_names = names;

    ControlUtil::setComboBoxTexts(m_comboBox, names);

    updateComboBoxIndex();
}

int EditCombo::value() const
{
    bool ok;
    const int v = m_lineEdit->text().trimmed().toInt(&ok);

    return (ok && v >= 0) ? v : -1;
}

void EditCombo::setupUi()
{
    setupEdit();
    setupCombo();

    auto layout = ControlUtil::createRowLayout(m_lineEdit, m_comboBox, /*stretch1=*/1);
    this->setLayout(layout);
}

void EditCombo::setupEdit()
{
    m_lineEdit = new LineEdit();
    m_lineEdit->setClearButtonEnabled(true);

    connect(m_lineEdit, &LineEdit::textChanged, this, &EditCombo::updateComboBoxIndex);
    connect(m_lineEdit, &LineEdit::textEdited, this, &EditCombo::edited);
}

void EditCombo::setupCombo()
{
    m_comboBox = ControlUtil::createComboBox();
    // The same width by any names
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_comboBox->setMinimumContentsLength(24);

    connect(m_comboBox, QOverload<int>::of(&QComboBox::activated), this,
            &EditCombo::updateEditText);
}

// The "Custom" item keeps the edit's text
void EditCombo::updateEditText(int index)
{
    if (index <= 0) {
        updateComboBoxIndex();
        return;
    }

    m_lineEdit->setText(QString::number(m_values.at(index)));

    emit itemSelected();
    emit edited();
}

void EditCombo::updateComboBoxIndex()
{
    const int index = m_values.indexOf(value());

    m_comboBox->setCurrentIndex(qMax(index, 0));
}
