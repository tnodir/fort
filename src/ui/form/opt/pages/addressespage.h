#ifndef ADDRESSESPAGE_H
#define ADDRESSESPAGE_H

#include "basepage.h"

class AddressesPage : public BasePage
{
    Q_OBJECT
public:
    explicit AddressesPage(OptionsController *ctrl = nullptr,
                           QWidget *parent = nullptr);

signals:

};

#endif // ADDRESSESPAGE_H
