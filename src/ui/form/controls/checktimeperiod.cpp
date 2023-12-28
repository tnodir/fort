#include "checktimeperiod.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimeEdit>

#include "controlutil.h"

CheckTimePeriod::CheckTimePeriod(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void CheckTimePeriod::setupUi()
{
    m_checkBox = new QCheckBox();

    m_timeEdit1 = createTimeEdit();
    m_timeEdit2 = createTimeEdit();

    auto layout = ControlUtil::createRowLayout(m_checkBox, m_timeEdit1);
    layout->addWidget(ControlUtil::createLabel("–"));
    layout->addWidget(m_timeEdit2);

    this->setLayout(layout);
}

QTimeEdit *CheckTimePeriod::createTimeEdit()
{
    auto c = new QTimeEdit();
    c->setFixedWidth(70);
    c->setDisplayFormat(timeFormat());
    c->setWrapping(true);
    return c;
}

QString CheckTimePeriod::fromTime(const QTime &time)
{
    return time.toString(timeFormat());
}

QTime CheckTimePeriod::toTime(const QString &text)
{
    if (text.isEmpty())
        return QTime(0, 0);

    return QTime::fromString(text, timeFormat());
}
