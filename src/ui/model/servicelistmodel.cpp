#include "servicelistmodel.h"

#include <QIcon>

#include <conf/appgroup.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <manager/serviceinfomanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

ServiceListModel::ServiceListModel(QObject *parent) : TableItemModel(parent) { }

ConfManager *ServiceListModel::confManager() const
{
    return IoC<ConfManager>();
}

FirewallConf *ServiceListModel::conf() const
{
    return confManager()->conf();
}

void ServiceListModel::initialize()
{
    m_services = ServiceInfoManager::loadServiceInfoList();

    reset();
}

int ServiceListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return services().size();
}

int ServiceListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
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
            return tr("Process ID");
        }
    }
    return {};
}

QVariant ServiceListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return dataDisplay(index);

    // Icon
    case Qt::DecorationRole:
        return dataDecoration(index);
    }

    return {};
}

QVariant ServiceListModel::dataDisplay(const QModelIndex &index) const
{
    const int row = index.row();
    const int column = index.column();

    const auto &info = serviceInfoAt(row);

    switch (column) {
    case 0:
        return info.realServiceName;
    case 1:
        return info.displayName;
    case 2:
        return dataDisplayProcessId(info);
    }

    return {};
}

QVariant ServiceListModel::dataDisplayProcessId(const ServiceInfo &info) const
{
    return (info.processId == 0) ? QVariant() : QVariant(info.processId);
}

QVariant ServiceListModel::dataDecoration(const QModelIndex &index) const
{
    const int column = index.column();

    if (column == 0) {
        const int row = index.row();

        const auto &info = serviceInfoAt(row);

        if (info.isTracked()) {
            return IconCache::icon(":/icons/widgets.png");
        }
    }

    return {};
}

Qt::ItemFlags ServiceListModel::flagIsEnabled(const QModelIndex &index) const
{
    const int row = index.row();

    const auto &info = serviceInfoAt(row);

    return info.isHostSplitDisabled ? Qt::NoItemFlags : Qt::ItemIsEnabled;
}

bool ServiceListModel::updateTableRow(const QVariantHash & /*vars*/, int /*row*/) const
{
    return true;
}

const ServiceInfo &ServiceListModel::serviceInfoAt(int index) const
{
    if (index < 0 || index >= services().size()) {
        static const ServiceInfo g_nullServiceInfo;
        return g_nullServiceInfo;
    }
    return services()[index];
}
