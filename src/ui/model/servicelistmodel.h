#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include <QVector>

#include <util/model/tableitemmodel.h>

class ConfManager;
class FirewallConf;
class ServiceInfo;
class ServiceInfoManager;

class ServiceListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject *parent = nullptr);

    ConfManager *confManager() const;
    ServiceInfoManager *serviceInfoManager() const;
    FirewallConf *conf() const;

    void initialize();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    bool updateTableRow(int row) const override;
    TableRow &tableRow() const override { return m_serviceRow; }

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDisplayAppGroup(const ServiceInfo &info) const;

private:
    mutable TableRow m_serviceRow;
};

#endif // SERVICELISTMODEL_H
