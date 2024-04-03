#ifndef TREEVIEW_H
#define TREEVIEW_H

#include <QTreeView>

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit TreeView(QWidget *parent = nullptr);

    QMenu *menu() const { return m_menu; }
    void setMenu(QMenu *menu) { m_menu = menu; }

    void setModel(QAbstractItemModel *model) override;

    void setupItemDelegate();

signals:
    void currentIndexChanged(const QModelIndex &index);

protected:
    void selectionChanged(
            const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu *m_menu = nullptr;
};

#endif // TREEVIEW_H
