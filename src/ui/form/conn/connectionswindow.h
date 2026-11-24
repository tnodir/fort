#ifndef CONNECTIONSWINDOW_H
#define CONNECTIONSWINDOW_H

#include <QPersistentModelIndex>

#include <functional>

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

class AppInfoRow;
class ConnSearchModel;
class ConnectionsController;
class HeaderButtons;
class TableView;

struct ConnRow;

class ConnectionsWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ConnectionsWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowConnections; }
    QString windowIconPath() const override { return ":/icons/connect.png"; }
    QString helpSection() const override { return "connections"; }
    /* The window's ini group: its state and List Options */
    QString iniGroup() const override;

    ConnectionsController *ctrl() const { return m_ctrl; }
    ConnSearchModel *connListModel() const { return m_connListModel; }

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

protected:
    static constexpr int connListHeaderVersion = 5;

    ConnectionsWindow(ConnSearchModel *connListModel, QWidget *parent, Qt::WindowFlags f);

    QHBoxLayout *headerLayout() const { return m_headerLayout; }
    TableView *connListView() const { return m_connListView; }
    QAction *addFilterAction() const { return m_actAddFilter; }

    void initialize();

    void restoreIniConnListHeader();

    void hideEditing();
    void hideClearAll();
    void hideAutoScroll();
    void hideLogConnIcons();

    /* Whether the shown connections are collected by the conf's flags */
    virtual bool logAllowedConn() const;
    virtual bool logBlockedConn() const;

    void updateLogConnIcons();

    int connListCurrentIndex() const;
    const ConnRow &currentConnRow() const;

    virtual void addConnFilter(const ConnRow &connRow);

    virtual QString clearAllConfirmText() const;
    virtual void clearAllConns();

protected slots:
    virtual void retranslateWindowTitle();

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupHeaderConnections();
    void setupConnRowAction(
            QAction *action, const std::function<void(const ConnRow &connRow)> &func);
    void setupEditSearch();
    void setupListOptions();
    void setupAutoScroll();
    void setupLogConnIcons();
    void setupTableConnList();
    void setupTableConnListHeader();
    void setupAppInfoRow();
    void setupTableConnsChanged();

    void showTableConnHeaderMenu(const QPoint &pos);

    void onTableConnSortClicked(int section, Qt::SortOrder order);
    void doAutoScroll();

    void saveTopRow();
    void restoreTopRow();

    void restoreIniListOptions();
    void updateAutoScroll();
    void setupAutoScrollConnections();
    void setupKeepScrollConnections();

    void deleteConn(int row);

    QString clearConnsConfirmText() const;
    void clearConns();

private:
    int m_topRowOffset = 0;

    ConnectionsController *m_ctrl = nullptr;
    ConnSearchModel *m_connListModel = nullptr;

    QHBoxLayout *m_headerLayout = nullptr;
    QPushButton *m_btEdit = nullptr;
    QAction *m_actAddFilter = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actCopyAsFilter = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actLookupIp = nullptr;
    QAction *m_actFilterSim = nullptr;
    QAction *m_actRemoveConn = nullptr;
    QAction *m_actClearAll = nullptr;
    QAction *m_actFind = nullptr;
    QToolButton *m_btClearAll = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QPushButton *m_btListOptions = nullptr;
    QCheckBox *m_cbAutoScroll = nullptr;
    QWidget *m_logConnIcons = nullptr;
    QLabel *m_iconLogAllowedConn = nullptr;
    QLabel *m_iconLogBlockedConn = nullptr;
    QLabel *m_iconLogAlertedConn = nullptr;
    HeaderButtons *m_headerButtons = nullptr;
    TableView *m_connListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    QPersistentModelIndex m_topRowIndex;

    QList<QMetaObject::Connection> m_scrollConnections;

    QList<QWidget *> m_editWidgets; // the Edit button and its separator
    QList<QWidget *> m_clearAllWidgets; // the Clear All button and its separator
};

#endif // CONNECTIONSWINDOW_H
