#ifndef CHECKSPINCOMBO_H
#define CHECKSPINCOMBO_H

#include "spincombo.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)

using ValuesList = QVector<int>;

class CheckSpinCombo : public SpinCombo
{
    Q_OBJECT

public:
    explicit CheckSpinCombo(QWidget *parent = nullptr);

    QCheckBox *checkBox() const { return m_checkBox; }

private:
    void setupUi();

private:
    QCheckBox *m_checkBox = nullptr;
};

#endif // CHECKSPINCOMBO_H
