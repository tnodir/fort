#ifndef CHECKSPINCOMBO_H
#define CHECKSPINCOMBO_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

using ValuesList = QVector<int>;

class CheckSpinCombo : public QWidget
{
    Q_OBJECT

public:
    explicit CheckSpinCombo(QWidget *parent = nullptr);

    const ValuesList &values() const { return m_values; }
    void setValues(const ValuesList &v);

    QStringList names() const { return m_names; }
    void setNames(const QStringList &v);

    QCheckBox *checkBox() const { return m_checkBox; }
    QSpinBox *spinBox() const { return m_spinBox; }
    QComboBox *comboBox() const { return m_comboBox; }

signals:
    void valuesChanged();
    void namesChanged();

private:
    void setupUi();
    void setupSpin();
    void setupCombo();

    void updateSpinBoxValue(int index);
    void updateComboBoxIndex(int value);
    int getIndexByValue(int value) const;

private:
    ValuesList m_values;
    QStringList m_names;

    QCheckBox *m_checkBox = nullptr;
    QSpinBox *m_spinBox = nullptr;
    QComboBox *m_comboBox = nullptr;
};

#endif // CHECKSPINCOMBO_H
