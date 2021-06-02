#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include <QVector>

#include "../util/model/tableitemmodel.h"

class ConfManager;
class FirewallConf;

struct ServiceInfo
{
    int groupIndex = 0;

    quint64 id = 0;

    QString serviceName;
    QString displayName;
};

class ServiceListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject *parent = nullptr);

    ConfManager *confManager() const;
    FirewallConf *conf() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_serviceRow; }

    const ServiceInfo &serviceInfoAt(int index) const;

private:
    QVector<ServiceInfo> m_services;

    mutable TableRow m_serviceRow;
};

#endif // SERVICELISTMODEL_H
