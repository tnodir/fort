#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include <QVector>

#include <util/model/tableitemmodel.h>
#include <util/service/serviceinfo.h>

class ConfManager;
class FirewallConf;
class ServiceInfo;

class ServiceListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject *parent = nullptr);

    ConfManager *confManager() const;
    FirewallConf *conf() const;

    void initialize();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const QVector<ServiceInfo> &services() const { return m_services; }
    const ServiceInfo &serviceInfoAt(int index) const;

protected:
    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_serviceRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

private:
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDisplayProcessId(const ServiceInfo &info) const;
    QVariant dataDecoration(const QModelIndex &index) const;

private:
    QVector<ServiceInfo> m_services;

    mutable TableRow m_serviceRow;
};

#endif // SERVICELISTMODEL_H
