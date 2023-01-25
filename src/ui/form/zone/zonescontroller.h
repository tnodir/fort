#ifndef ZONESCONTROLLER_H
#define ZONESCONTROLLER_H

#include <form/basecontroller.h>

class ZoneListModel;

class ZonesController : public BaseController
{
    Q_OBJECT

public:
    explicit ZonesController(QObject *parent = nullptr);

    ZoneListModel *zoneListModel() const;

signals:
    void retranslateUi();
};

#endif // ZONESCONTROLLER_H
