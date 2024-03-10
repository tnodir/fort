#ifndef FTSTABLESQLMODEL_H
#define FTSTABLESQLMODEL_H

#include "tablesqlmodel.h"

class FtsTableSqlModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit FtsTableSqlModel(QObject *parent = nullptr);

    QString ftsFilterMatch() const { return m_ftsFilterMatch; }
    QString ftsFilter() const { return m_ftsFilter; }
    void setFtsFilter(const QString &filter);

protected:
    void fillQueryVars(QVariantHash &vars) const override;

    QString sqlWhere() const override;
    virtual QString sqlWhereFts() const = 0;

private:
    QString m_ftsFilter;
    QString m_ftsFilterMatch;
};

#endif // FTSTABLESQLMODEL_H
