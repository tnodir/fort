#ifndef TRAFLISTMODEL_H
#define TRAFLISTMODEL_H

#include <QAbstractItemModel>

QT_FORWARD_DECLARE_CLASS(DatabaseManager)

struct TrafficRow {
    bool isValid(int row) const { return row == this->row; }
    void invalidate() { row = -1; }

    int row;
    qint32 trafTime;
    qint64 inBytes;
    qint64 outBytes;
};

class TrafListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum TrafType {
        TrafHourly = 0,
        TrafDaily,
        TrafMonthly,
        TrafTotal
    };
    Q_ENUM(TrafType)

    explicit TrafListModel(DatabaseManager *databaseManager,
                           QObject *parent = nullptr);

    TrafListModel::TrafType type() const { return m_type; }
    void setType(TrafListModel::TrafType type);

    qint64 appId() const { return m_appId; }
    void setAppId(qint64 appId);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QModelIndex sibling(int row, int column,
                        const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:

public slots:
    void clear();

    void resetAppTotals();

    void reset();
    void refresh();

private:
    void invalidateRowCache();
    void updateRowCache(int row) const;

    QString formatTrafUnit(qint64 bytes) const;
    QString formatTrafTime(qint32 trafTime) const;

    qint32 getTrafTime(int row) const;

    static qint32 getTrafCount(TrafType type, qint32 minTrafTime,
                               qint32 maxTrafTime);
    static qint32 getMaxTrafTime(TrafType type);

    static const char *getSqlMinTrafTime(TrafType type, qint64 appId);
    static const char *getSqlSelectTraffic(TrafType type, qint64 appId);

private:
    bool m_isEmpty;

    TrafType m_type;

    qint64 m_appId;

    qint32 m_minTrafTime;
    qint32 m_maxTrafTime;
    qint32 m_trafCount;

    mutable TrafficRow m_rowCache;

    DatabaseManager *m_databaseManager;
};

#endif // TRAFLISTMODEL_H
