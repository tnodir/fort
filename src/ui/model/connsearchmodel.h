#ifndef CONNSEARCHMODEL_H
#define CONNSEARCHMODEL_H

#include <QVector>

#include <util/textmatcher.h>
#include <util/triggertimer.h>

#include "connlistmodel.h"

class ConnSearchModel : public ConnListModel
{
    Q_OBJECT

public:
    explicit ConnSearchModel(QObject *parent = nullptr);

    const QString &textFilter() const { return m_textFilter; }
    void setTextFilter(const QString &filter);

    /* The search is applied */
    bool isFiltering() const { return !m_textMatcher.isEmpty(); }

public slots:
    void updateSearchLater();

protected slots:
    void updateConnIdRange() override;
    void clearConnRows() override;

protected:
    static constexpr int searchConnsMax = 5000;

    bool updateTableRow(const QVariantHash &vars, int row) const override;

    int doSqlCount() const override;

    /* The rows are searched and loaded */
    virtual bool isSearching() const { return isFiltering(); }

    /* The first of the last connections to search */
    virtual qint64 lastConnsIdMin(qint64 idMax) const;

    virtual QString sqlSearchWhere() const;
    virtual void fillSearchVars(QVariantList & /*vars*/) const { }

private slots:
    void updateSearch();

private:
    qint64 searchConnIdMin();

    void removeConnRowsBefore(qint64 idMin);
    void removeFirstConnRows(int count);
    void appendConnRows(qint64 connIdFrom);

    void loadConnRows(qint64 connIdFrom, QVector<ConnRow> &connRows);

private:
    qint64 m_lastConnId = 0; // the last loaded conn_id

    QString m_textFilter;

    TextMatcher m_textMatcher;

    QVector<ConnRow> m_connRows; // matched, ascending by conn_id

    TriggerTimer m_searchTimer;
};

#endif // CONNSEARCHMODEL_H
