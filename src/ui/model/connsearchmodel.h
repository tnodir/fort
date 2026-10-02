#ifndef CONNSEARCHMODEL_H
#define CONNSEARCHMODEL_H

#include <QRegularExpression>
#include <QVector>

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
    bool isFiltering() const { return !m_words.isEmpty() || !m_regexp.pattern().isEmpty(); }

public slots:
    void updateSearchLater();

protected slots:
    void updateConnIdRange() override;
    void clearConnRows() override;

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;

    int doSqlCount() const override;

private slots:
    void updateSearch();

private:
    void setupTerms();

    void removeConnRowsBefore(qint64 idMin);
    void removeFirstConnRows(int count);
    void appendConnRows(qint64 connIdFrom);

    void loadConnRows(qint64 connIdFrom, QVector<ConnRow> &connRows);

    bool isTextMatched(const QString &text) const;

private:
    qint64 m_lastConnId = 0; // the last loaded conn_id

    QString m_textFilter;

    QRegularExpression m_regexp;
    QStringList m_words;

    QVector<ConnRow> m_connRows; // matched, ascending by conn_id

    TriggerTimer m_searchTimer;
};

#endif // CONNSEARCHMODEL_H
