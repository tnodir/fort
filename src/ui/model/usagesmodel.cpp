#include "usagesmodel.h"

#include <QFont>

#include <conf/firewallconf.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/model/tableitemmodel.h>
#include <util/stringutil.h>

#include "rulelistmodel.h"

// A root's internal id is 0, a usage's one is its root's row + 1

UsagesModel::UsagesModel(QObject *parent) : QAbstractItemModel(parent) { }

void UsagesModel::setUsages(const ConfUsages &usages)
{
    beginResetModel();

    m_usages = usages;

    m_categories.clear();
    for (int i = 0; i < ConfUsages::CategoryCount; ++i) {
        const auto category = ConfUsages::Category(i);

        if (!categoryList(category).isEmpty()) {
            m_categories.append(category);
        }
    }

    endResetModel();
}

bool UsagesModel::isIndexRoot(const QModelIndex &index)
{
    return index.isValid() && index.internalId() == 0;
}

ConfUsages::Category UsagesModel::indexCategory(const QModelIndex &index) const
{
    const int rootRow = isIndexRoot(index) ? index.row() : int(index.internalId()) - 1;

    return m_categories.at(rootRow);
}

const ConfUsage &UsagesModel::usageAt(const QModelIndex &index) const
{
    return categoryList(indexCategory(index)).at(index.row());
}

QModelIndex UsagesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return {};

    const quintptr internalId = parent.isValid() ? quintptr(parent.row() + 1) : 0;

    return createIndex(row, column, internalId);
}

QModelIndex UsagesModel::parent(const QModelIndex &child) const
{
    if (!child.isValid() || isIndexRoot(child))
        return {};

    return createIndex(int(child.internalId()) - 1, 0, quintptr(0));
}

int UsagesModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_categories.size();
    }

    if (!isIndexRoot(parent) || parent.column() > 0)
        return 0;

    return categoryList(indexCategory(parent)).size();
}

int UsagesModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant UsagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    return isIndexRoot(index) ? rootData(index, role) : usageData(index, role);
}

void UsagesModel::refresh()
{
    emit layoutAboutToBeChanged();
    emit layoutChanged();
}

const ConfUsageList &UsagesModel::categoryList(ConfUsages::Category category) const
{
    return m_usages.lists[category];
}

QVariant UsagesModel::rootData(const QModelIndex &index, int role) const
{
    const auto category = indexCategory(index);

    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return rootDisplay(category);

    // Icon
    case Qt::DecorationRole:
        return IconCache::icon(categoryIconPath(category));

    // Font
    case Qt::FontRole:
        return GuiUtil::fontBold();
    }

    return {};
}

QVariant UsagesModel::usageData(const QModelIndex &index, int role) const
{
    switch (role) {
    // Label
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return usageDisplay(indexCategory(index), usageAt(index));

    // Enabled
    case TableItemModel::EnabledRole:
        return true;
    }

    return {};
}

QString UsagesModel::rootDisplay(ConfUsages::Category category) const
{
    return StringUtil::textWithCount(categoryName(category), categoryList(category).size());
}

QString UsagesModel::usageDisplay(ConfUsages::Category category, const ConfUsage &usage)
{
    if (category == ConfUsages::CategoryAddressLists) {
        return FirewallConf::addressListNames().value(usage.id);
    }

    return usage.name;
}

QString UsagesModel::categoryName(ConfUsages::Category category)
{
    // Sync with enum ConfUsages::Category
    const QStringList names = QStringList { tr("IP Addresses"), tr("Programs"), tr("Groups") }
            + RuleListModel::ruleTypeNames();

    return names.value(category);
}

QString UsagesModel::categoryIconPath(ConfUsages::Category category)
{
    // Sync with enum ConfUsages::Category
    static const QStringList iconPaths = QStringList {
        ":/icons/ip.png", ":/icons/application.png", ":/icons/application_double.png"
    } + RuleListModel::ruleTypeIconPaths();

    return iconPaths.value(category);
}
