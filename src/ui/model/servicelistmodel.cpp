#include "servicelistmodel.h"

#include "../conf/appgroup.h"
#include "../conf/confmanager.h"
#include "../conf/firewallconf.h"
#include "../util/ioc/ioccontainer.h"

ServiceListModel::ServiceListModel(QObject *parent) : TableItemModel(parent) { }

ConfManager *ServiceListModel::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *ServiceListModel::conf() const
{
    return confManager()->conf();
}

int ServiceListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_services.size();
}

int ServiceListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant ServiceListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::ToolTipRole)) {
        switch (section) {
        case 0:
            return tr("Service Name");
        case 1:
            return tr("Display Name");
        case 2:
            return tr("Group");
        }
    }
    return QVariant();
}

QVariant ServiceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
        const int row = index.row();
        const int column = index.column();

        const auto info = serviceInfoAt(row);

        switch (column) {
        case 0:
            return info.serviceName;
        case 1:
            return info.displayName;
        case 2:
            return conf()->appGroupAt(info.groupIndex)->name();
        }
    }

    return QVariant();
}

bool ServiceListModel::updateTableRow(int /*row*/) const
{
    return true;
}

const ServiceInfo &ServiceListModel::serviceInfoAt(int index) const
{
    if (index < 0 || index >= m_services.size()) {
        static const ServiceInfo g_nullServiceInfo;
        return g_nullServiceInfo;
    }
    return m_services[index];
}
