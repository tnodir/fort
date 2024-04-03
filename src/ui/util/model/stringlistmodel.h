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

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    const QStringList &list() const { return m_list; }
    void setList(const QStringList &list);

public slots:
    virtual void clear();

    virtual void insert(const QString &text, int row = -1);
    virtual void remove(int row = -1);
    virtual void replace(const QString &text, int row = -1);

    bool canMove(int fromRow, int toRow) const;
    virtual void move(int fromRow, int toRow);

    void reset();
    void refresh();

protected:
    void removeRow(int row);

    int adjustRow(int row) const;

    void doBeginRemoveRows(int first, int last, const QModelIndex &parent = {});
    void doEndRemoveRows();

    bool isChanging() const { return m_isChanging; }
    void setIsChanging(bool v) { m_isChanging = v; }

private:
    bool m_isChanging = false;

    QStringList m_list;
};

#endif // STRINGLISTMODEL_H
