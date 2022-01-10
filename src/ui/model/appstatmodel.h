#ifndef APPSTATMODEL_H
#define APPSTATMODEL_H

#include <util/model/stringlistmodel.h>

class AppInfoCache;
class StatManager;

class AppStatModel : public StringListModel
{
    Q_OBJECT

public:
    explicit AppStatModel(QObject *parent = nullptr);

    StatManager *statManager() const;
    AppInfoCache *appInfoCache() const;

    void initialize();

    qint64 appIdByRow(int row) const;
    QString appPathByRow(int row) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void clear() override;

    void remove(int row = -1) override;

private slots:
    void onStatAppRemoved(qint64 appId);

    void handleCreatedApp(qint64 appId, const QString &appPath);

private:
    void updateList();

    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDecoration(const QModelIndex &index) const;

private:
    QVector<qint64> m_appIds;
};

#endif // APPSTATMODEL_H
