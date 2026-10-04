#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include <QVector>

#include <util/model/tableitemmodel.h>
#include <util/service/serviceinfo.h>
#include <util/textmatcher.h>

class ServiceInfo;

class ServiceListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit ServiceListModel(QObject *parent = nullptr);

    void initialize();

    const QString &textFilter() const { return m_textFilter; }
    void setTextFilter(const QString &filter);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    const QVector<ServiceInfo> &services() const { return m_services; }
    const ServiceInfo &serviceInfoAt(int index) const;

protected:
    Qt::ItemFlags flagIsEnabled(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_serviceRow; }

    void fillQueryVarsForRow(QVariantHash & /*vars*/, int /*row*/) const override { }

private:
    void updateServices();
    void sortServices();

    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataDisplayProcessId(const ServiceInfo &info) const;
    QVariant dataToolTip(const QModelIndex &index) const;
    QVariant dataDecoration(const QModelIndex &index) const;

    QString trackStatusText(const ServiceInfo &info) const;

    static QString trackIconPath(const ServiceInfo &info);

private:
    int m_sortColumn = -1;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QString m_textFilter;

    TextMatcher m_textMatcher;

    QVector<ServiceInfo> m_allServices;
    QVector<ServiceInfo> m_services; // matched

    mutable TableRow m_serviceRow;
};

#endif // SERVICELISTMODEL_H
