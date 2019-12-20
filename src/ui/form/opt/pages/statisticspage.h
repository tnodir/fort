#ifndef STATISTICSPAGE_H
#define STATISTICSPAGE_H

#include "basepage.h"

class StatisticsPage : public BasePage
{
    Q_OBJECT

public:
    explicit StatisticsPage(OptionsController *ctrl = nullptr,
                            QWidget *parent = nullptr);

signals:

};

#endif // STATISTICSPAGE_H
