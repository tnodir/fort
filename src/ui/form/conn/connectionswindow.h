#ifndef CONNECTIONSWINDOW_H
#define CONNECTIONSWINDOW_H

#include <QPersistentModelIndex>

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QHeaderView)

class AppInfoRow;
class ConnSearchModel;
class ConnectionsController;
class TableView;

struct ConnRow;

class ConnectionsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ConnectionsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowConnections; }
    QString windowOverlayIconPath() const override { return ":/icons/connect.png"; }

    ConnectionsController *ctrl() const { return m_ctrl; }
    ConnSearchModel *connListModel() const { return m_connListModel; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    static constexpr int connListHeaderVersion = 5;

    ConnectionsWindow(ConnSearchModel *connListModel, QWidget *parent, Qt::WindowFlags f);

    QHBoxLayout *headerLayout() const { return m_headerLayout; }
    TableView *connListView() const { return m_connListView; }

    void initialize();

    void hideEditing();

    int connListCurrentIndex() const;
    const ConnRow &currentConnRow() const;

protected slots:
    virtual void retranslateWindowTitle();

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupHeaderConnections();
    void setupEditSearch();
    void setupListOptions();
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

private:
    int m_topRowOffset = 0;

    ConnectionsController *m_ctrl = nullptr;
    ConnSearchModel *m_connListModel = nullptr;

    QHBoxLayout *m_headerLayout = nullptr;
    QPushButton *m_btEdit = nullptr;
    QAction *m_actCopyAsFilter = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actLookupIp = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actFilterSim = nullptr;
    QAction *m_actRemoveConn = nullptr;
    QAction *m_actClearAll = nullptr;
    QAction *m_actFind = nullptr;
    QToolButton *m_btClearAll = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btListOptions = nullptr;
    QCheckBox *m_cbAutoScroll = nullptr;
    QCheckBox *m_cbShowHostNames = nullptr;
    QToolButton *m_btOptions = nullptr;
    QToolButton *m_btStatistics = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_connListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    QPersistentModelIndex m_topRowIndex;

    QList<QWidget *> m_editWidgets; // the Edit, Clear All and List Options buttons
};

#endif // CONNECTIONSWINDOW_H
