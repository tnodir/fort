#include "servicelistmodel.h"

#include <QIcon>

#include <fortglobal.h>
#include <manager/serviceinfomanager.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>

using namespace Fort;

namespace {

QString serviceSearchText(const ServiceInfo &info)
{
    QStringList list = { info.realServiceName, info.displayName };

    if (info.processId != 0) {
        list.append(QString::number(info.processId));
    }

    return list.join('\n');
}

}

ServiceListModel::ServiceListModel(QObject *parent) : TableItemModel(parent) { }

void ServiceListModel::initialize()
{
    m_allServices = ServiceInfoManager::loadServiceInfoList();

    updateServices();
}

void ServiceListModel::setTextFilter(const QString &filter)
{
    if (m_textFilter == filter)
        return;

    m_textFilter = filter;
    m_textMatcher.setFilter(filter);

    updateServices();
}

void ServiceListModel::updateServices()
{
    m_services.clear();

    for (const auto &info : std::as_const(m_allServices)) {
        if (m_textMatcher.isMatched(serviceSearchText(info))) {
            m_services.append(info);
        }
    }

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
    if (orientation != Qt::Horizontal)
        return {};

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return headerDataDisplay(section);
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

QVariant ServiceListModel::headerDataDisplay(int section) const
{
    switch (section) {
    case 0:
        return tr("Service Name");
    case 1:
        return tr("Display Name");
    case 2:
        return tr("Process ID");
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
            return IconCache::icon(info.isOwnProcess() ? ":/icons/tick.png" : ":/icons/cross.png");
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
