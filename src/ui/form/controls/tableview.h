#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QTableView>

#include <util/model/tablesqlmodel.h>

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

    /* The rows' fixed height, 0 - by the font, icon size and style */
    void setRowsHeight(int v);

    /* Select the row (or the last one) after the model's next reset */
    void selectRowOnReset(int row) { m_resetRow = row; }

    /* Keep the selected rows by their ids (the first column of the model's SELECT):
     * restore them after the model's reset or refresh; the model must be set already */
    void keepSelectionByIds();

signals:
    void currentIndexChanged(const QModelIndex &index);

public slots:
    void selectCell(int row, int column = 0);

    void copySelectedText();

protected:
    void changeEvent(QEvent *event) override;

    void selectionChanged(
            const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    void contextMenuEvent(QContextMenuEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

private:
    void onModelReset();

    void selectResetRow();

    void rememberSelection();
    void restoreSelection();
    void selectRowsByIds(const QVector<TableSqlModel::IdRow> &idRows, qint64 currentId);

    int rowScrollPos(int row) const;

    void updateRowsHeight();
    int defaultRowsHeight() const;

private:
    int m_resetRow = -1;
    int m_rowsHeight = 0;

    qint64 m_keptCurrentId = 0;

    QMenu *m_menu = nullptr;
    TableSqlModel *m_keptModel = nullptr;

    QVector<qint64> m_keptIds;
};

#endif // TABLEVIEW_H
