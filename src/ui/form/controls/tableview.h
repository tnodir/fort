#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QTableView>

class TableView : public QTableView
{
    Q_OBJECT

public:
    explicit TableView(QWidget *parent = nullptr);

    QMenu *menu() const { return m_menu; }
    void setMenu(QMenu *menu) { m_menu = menu; }

    void setModel(QAbstractItemModel *model) override;

    int currentRow() const;
    QVector<int> selectedRows() const;
    QModelIndexList sortedSelectedIndexes() const;

    QString selectedText() const;
    QString cellText(const QModelIndex &index) const;

    /* The vertical scroll position relative to the row */
    int scrollOffset(int row) const;
    void setScrollOffset(int row, int offset);

signals:
    void currentIndexChanged(const QModelIndex &index);

public slots:
    void selectCell(int row, int column = 0);

    void copySelectedText();

protected:
    void selectionChanged(
            const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    int rowScrollPos(int row) const;

private:
    QMenu *m_menu = nullptr;
};

#endif // TABLEVIEW_H
