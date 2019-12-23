#include "checktimeperiod.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimeEdit>

CheckTimePeriod::CheckTimePeriod(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void CheckTimePeriod::setupUi()
{
    auto layout = new QHBoxLayout();

    m_checkBox = new QCheckBox();

    m_timeEdit1 = createTimeEdit();
    m_timeEdit2 = createTimeEdit();

    layout->addWidget(m_checkBox);
    layout->addWidget(m_timeEdit1);
    layout->addWidget(new QLabel("-"));
    layout->addWidget(m_timeEdit2);

    this->setLayout(layout);
}

QTimeEdit *CheckTimePeriod::createTimeEdit()
{
    auto c = new QTimeEdit();
    c->setMinimumWidth(70);
    c->setDisplayFormat(timeFormat());
    c->setWrapping(true);
    return c;
}
