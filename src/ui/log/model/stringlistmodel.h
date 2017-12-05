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

protected:
    void insert(const QString &text, int row = -1);
    void remove(int row = -1);
    void replace(const QString &text, int row = -1);

signals:

public slots:
    virtual void clear();

private:
    int adjustRow(int row) const;

private:
    QStringList m_list;
};

#endif // STRINGLISTMODEL_H
