#ifndef GRAPHLISTMODEL_H
#define GRAPHLISTMODEL_H

#include <QVector>

#include <util/model/tableitemmodel.h>

enum class GraphListColumn : qint8 {
    Name = 0,
    GraphType,
    Count,
};

struct GraphRow : TableRow
{
    bool visible = false;

    int graphId = 0;
    int graphType = 0;

    QString name;
};

class GraphListModel : public TableItemModel
{
    Q_OBJECT

public:
    explicit GraphListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(
            int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    const GraphRow &graphRowAt(int row) const;

    static QString columnName(GraphListColumn column);

public slots:
    void reset() override;

protected:
    Qt::ItemFlags flagIsUserCheckable(const QModelIndex &index) const override;

    bool updateTableRow(const QVariantHash &vars, int row) const override;
    TableRow &tableRow() const override { return m_graphRow; }

private:
    void setupGraphIds();

    QVariant headerDataDisplay(int section) const;
    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant dataCheckState(const QModelIndex &index) const;

private:
    QVector<int> m_graphIds;

    mutable GraphRow m_graphRow;
};

#endif // GRAPHLISTMODEL_H
