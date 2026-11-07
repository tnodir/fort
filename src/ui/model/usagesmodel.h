#ifndef USAGESMODEL_H
#define USAGESMODEL_H

#include <QAbstractItemModel>
#include <QVector>

#include <conf/confusages.h>

class UsagesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit UsagesModel(QObject *parent = nullptr);

    void setUsages(const ConfUsages &usages);

    static bool isIndexRoot(const QModelIndex &index);

    ConfUsages::Category indexCategory(const QModelIndex &index) const;
    const ConfUsage &usageAt(const QModelIndex &index) const;

    QModelIndex index(
            int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void refresh();

private:
    const ConfUsageList &categoryList(ConfUsages::Category category) const;

    QVariant rootData(const QModelIndex &index, int role) const;
    QVariant usageData(const QModelIndex &index, int role) const;

    QString rootDisplay(ConfUsages::Category category) const;

    static QString usageDisplay(ConfUsages::Category category, const ConfUsage &usage);

    static QString categoryName(ConfUsages::Category category);
    static QString categoryIconPath(ConfUsages::Category category);

private:
    QVector<ConfUsages::Category> m_categories; // the non-empty ones

    ConfUsages m_usages;
};

#endif // USAGESMODEL_H
