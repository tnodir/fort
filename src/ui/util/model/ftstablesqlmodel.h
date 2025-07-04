#ifndef FTSTABLESQLMODEL_H
#define FTSTABLESQLMODEL_H

#include "tablesqlmodel.h"

class FtsTableSqlModel : public TableSqlModel
{
    Q_OBJECT

public:
    explicit FtsTableSqlModel(QObject *parent = nullptr);

    const QString &textFilter() const { return m_textFilter; }
    void setTextFilter(const QString &filter);

protected:
    void setupRegexpFilter();
    void setupFtsFilter();

    const QString &regexpFilterMatch() const { return m_regexpFilterMatch; }
    const QString &regexpFilterColumns() const { return m_regexpFilterColumns; }

    const QString &ftsFilterMatch() const { return m_ftsFilterMatch; }

protected:
    void fillQueryVars(QVariantHash &vars) const override;

    QString sqlWhere() const override;
    virtual QString sqlWhereRegexp() const;
    virtual QString sqlWhereFts() const = 0;

    virtual const QStringList &regexpColumns() const = 0;

private:
    QString m_textFilter;

    QString m_regexpFilterMatch;
    QString m_regexpFilterColumns;

    QString m_ftsFilterMatch;
};

#endif // FTSTABLESQLMODEL_H
