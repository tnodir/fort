#ifndef CONNECTIONSPAGE_H
#define CONNECTIONSPAGE_H

#include <QPersistentModelIndex>

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QHeaderView)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppInfoRow;
class ConnSearchModel;
class IniUser;
class StatisticsController;
class TableView;

struct ConnRow;

class ConnectionsPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit ConnectionsPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

    ConnSearchModel *connListModel() const { return m_connListModel; }

protected slots:
    void onSaveWindowState(IniUser &ini) override;
    void onRestoreWindowState(IniUser &ini) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupHeaderConnections();
    void setupEditSearch();
    void setupOptions();
    void setupAutoScroll();
    void setupShowHostNames();
    void setupTableConnList();
    void setupTableConnListHeader();
    void setupAppInfoRow();
    void setupTableConnsChanged();

    void showTableConnHeaderMenu(const QPoint &pos);
    void setupTableConnHeaderMenuColumns(QMenu *menu, QHeaderView *header);

    void onTableConnSortClicked(int section, Qt::SortOrder order);
    void doAutoScroll();

    void saveTopRow();
    void restoreTopRow();

    void updateAutoScroll();
    void setupAutoScrollConnections();
    void setupKeepScrollConnections();
    void updateShowHostNames();

    void deleteConn(int row);

    int connListCurrentIndex() const;
    const ConnRow &currentConnRow() const;

private:
    int m_topRowOffset = 0;

    ConnSearchModel *m_connListModel = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actCopyAsFilter = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actLookupIp = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actRemoveConn = nullptr;
    QAction *m_actClearAll = nullptr;
    QAction *m_actFind = nullptr;
    QToolButton *m_btClearAll = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btOptions = nullptr;
    QCheckBox *m_cbAutoScroll = nullptr;
    QCheckBox *m_cbShowHostNames = nullptr;
    TableView *m_connListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    QPersistentModelIndex m_topRowIndex;
};

#endif // CONNECTIONSPAGE_H
