#ifndef SPINCOMBO_H
#define SPINCOMBO_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

using ValuesList = QVector<int>;

class SpinCombo : public QWidget
{
    Q_OBJECT

public:
    explicit SpinCombo(QWidget *parent = nullptr);

    const ValuesList &values() const { return m_values; }
    void setValues(const ValuesList &v);

    QStringList names() const { return m_names; }
    void setNames(const QStringList &v);

    void setNamesByValues();

    QHBoxLayout *boxLayout() const;
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

    QSpinBox *m_spinBox = nullptr;
    QComboBox *m_comboBox = nullptr;
};

#endif // SPINCOMBO_H
