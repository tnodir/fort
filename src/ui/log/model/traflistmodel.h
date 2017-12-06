#ifndef TRAFLISTMODEL_H
#define TRAFLISTMODEL_H

#include <QAbstractListModel>

QT_FORWARD_DECLARE_CLASS(DatabaseManager)

class TrafListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum TrafRole {
        DateTimeRole = Qt::DisplayRole,
        DownloadRole = Qt::UserRole,
        UploadRole,
        SumRole
    };
    Q_ENUM(TrafRole)

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

    QString appPath() const { return m_appPath; }
    void setAppPath(const QString &appPath);

    void reset();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:

public slots:
    void clear();

private:
    QString formatTrafTime(qint32 trafTime) const;

    qint32 getTrafTime(int row) const;

    static qint32 getTrafCount(TrafType type, qint32 minTrafTime,
                               qint32 maxTrafTime);
    static qint32 getMaxTrafTime(TrafType type);

    static const char *getSqlMinTrafTime(TrafType type, qint64 appId);
    static const char *getSqlSelectTraffic(TrafType type, qint64 appId);

private:
    TrafType m_type;
    QString m_appPath;

    qint64 m_appId;

    qint32 m_minTrafTime;
    qint32 m_maxTrafTime;
    qint32 m_trafCount;

    DatabaseManager *m_databaseManager;
};

#endif // TRAFLISTMODEL_H
