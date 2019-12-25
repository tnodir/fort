#ifndef LABELSPINCOMBO_H
#define LABELSPINCOMBO_H

#include "spincombo.h"

QT_FORWARD_DECLARE_CLASS(QLabel)

class LabelSpinCombo : public SpinCombo
{
    Q_OBJECT

public:
    explicit LabelSpinCombo(QWidget *parent = nullptr);

    QLabel *label() const { return m_label; }

private:
    void setupUi();

private:
    QLabel *m_label = nullptr;
};

#endif // LABELSPINCOMBO_H
