#ifndef SCHEDULEPAGE_H
#define SCHEDULEPAGE_H

#include "basepage.h"

class SchedulePage : public BasePage
{
    Q_OBJECT

public:
    explicit SchedulePage(OptionsController *ctrl = nullptr,
                          QWidget *parent = nullptr);

signals:

};

#endif // SCHEDULEPAGE_H
