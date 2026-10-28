#ifndef PENDINGCONNSEARCHMODEL_H
#define PENDINGCONNSEARCHMODEL_H

#include "connsearchmodel.h"

/* The pending (Ask to Connect) connections of all programs */
class PendingConnSearchModel : public ConnSearchModel
{
    Q_OBJECT

public:
    explicit PendingConnSearchModel(QObject *parent = nullptr);

protected:
    StatConnManager *connManager() const override;

    /* The pending connections, not timed out yet */
    QString sqlSearchWhere() const override;
    void fillSearchVars(QVariantList &vars) const override;
};

#endif // PENDINGCONNSEARCHMODEL_H
