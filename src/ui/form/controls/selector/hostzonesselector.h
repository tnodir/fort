#ifndef HOSTZONESSELECTOR_H
#define HOSTZONESSELECTOR_H

#include "zonesselector.h"

// Lists the Zones of the Hosts types only
class HostZonesSelector : public ZonesSelector
{
    Q_OBJECT

public:
    explicit HostZonesSelector(QWidget *parent = nullptr);

    void retranslateUi() override;

protected:
    quint32 listedMask() const override;
};

#endif // HOSTZONESSELECTOR_H
