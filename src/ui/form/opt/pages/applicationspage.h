#ifndef APPLICATIONSPAGE_H
#define APPLICATIONSPAGE_H

#include "basepage.h"

class ApplicationsPage : public BasePage
{
    Q_OBJECT

public:
    explicit ApplicationsPage(OptionsController *ctrl = nullptr,
                              QWidget *parent = nullptr);

signals:

};

#endif // APPLICATIONSPAGE_H
