#ifndef STRINGLISTMODEL_H
#define STRINGLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>

class StringListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit StringListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    const QStringList &list() const { return m_list; }
    void setList(const QStringList &list);

signals:

public slots:
    virtual void clear();

    virtual void insert(const QString &text, int row = -1);
    virtual void remove(int row = -1);
    virtual void replace(const QString &text, int row = -1);

    void reset()
    {
        beginResetModel();
        endResetModel();
    }
    void refresh();

protected:
    void removeRow(int row);

    int adjustRow(int row) const;

private:
    QStringList m_list;
};

#endif // STRINGLISTMODEL_H
