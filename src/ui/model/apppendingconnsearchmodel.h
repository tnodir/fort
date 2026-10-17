#ifndef APPPENDINGCONNSEARCHMODEL_H
#define APPPENDINGCONNSEARCHMODEL_H

#include <QIcon>
#include <QSet>
#include <QTimer>

#include "appconnsearchmodel.h"

/* The program's pending connections listed since the window's opening: the no longer pending
 * (decided or timed out) ones are kept */
class AppPendingConnSearchModel : public AppConnSearchModel
{
    Q_OBJECT

public:
    explicit AppPendingConnSearchModel(QObject *parent = nullptr);

    /* The listed connection is still pending */
    bool isConnPending(int row) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected slots:
    /* The listed rows are kept: the new ones are appended */
    void updateConnIdRange() override;
    void clearConnRows() override;

    /* The listed rows are filtered: the no longer pending ones aren't in the DB */
    void updateSearch() override;

protected:
    StatConnManager *connManager() const override;

    /* The program's pending (Ask to Connect) connections, not timed out yet */
    QString sqlSearchWhere() const override;
    void fillSearchVars(QVariantList &vars) const override;

private:
    void setupTimer();

    void updateTimer();
    void updateTimeoutsProgress();

    bool isListedApp() const;
    void setListedApp();

    void updatePendingConnIds();
    void removeExcessConnRows();
    void removeListedConnRowAt(int index);

    bool hasPendingConns() const;

    QIcon actionIcon(int row) const;
    QString actionToolTip(int row) const;

    qint64 timeoutSeconds(int row) const;
    int timeoutPercent(int row) const;
    QString timeoutText(int row) const;

private:
    qint64 m_listedConfAppId = 0;

    QString m_listedAppPath;

    QVector<ConnRow> m_listedRows; // the matched and not matched ones, ascending by conn_id

    QSet<qint64> m_pendingConnIds; // of the listed connections

    QTimer m_timer;
};

#endif // APPPENDINGCONNSEARCHMODEL_H
