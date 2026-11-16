#ifndef EDITCOMBO_H
#define EDITCOMBO_H

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QComboBox)

class LineEdit;

// The edit's values with the combo box's named ones: the first item is "Custom", for the other
// values, e.g. a list or a range
class EditCombo : public QWidget
{
    Q_OBJECT

public:
    using ValuesList = QVector<int>;

    explicit EditCombo(QWidget *parent = nullptr);

    const ValuesList &values() const { return m_values; }
    const QStringList &names() const { return m_names; }
    void setItems(const ValuesList &values, const QStringList &names);

    LineEdit *lineEdit() const { return m_lineEdit; }
    QComboBox *comboBox() const { return m_comboBox; }

    // The edit's text as one number, else -1
    int value() const;

signals:
    // By the user: the edit's text or the combo box's item
    void edited();

    // By the user: the combo box's named item, before edited()
    void itemSelected();

private:
    void setupUi();
    void setupEdit();
    void setupCombo();

    void updateEditText(int index);
    void updateComboBoxIndex();

private:
    ValuesList m_values;
    QStringList m_names;

    LineEdit *m_lineEdit = nullptr;
    QComboBox *m_comboBox = nullptr;
};

#endif // EDITCOMBO_H
