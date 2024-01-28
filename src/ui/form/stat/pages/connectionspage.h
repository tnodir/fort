#ifndef CONNECTIONSPAGE_H
#define CONNECTIONSPAGE_H

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppInfoCache;
class AppInfoRow;
class ConfManager;
class ConnBlockListModel;
class FirewallConf;
class FortManager;
class FortSettings;
class IniUser;
class StatisticsController;
class TableView;

class ConnectionsPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit ConnectionsPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

    ConnBlockListModel *connBlockListModel() const { return m_connBlockListModel; }
    AppInfoCache *appInfoCache() const;

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupOptions();
    void setupAutoScroll();
    void setupShowHostNames();
    void setupTableConnList();
    void setupTableConnListHeader();
    void setupAppInfoRow();
    void setupTableConnsChanged();

    void updateAutoScroll();
    void updateShowHostNames();

    void deleteConn(int row);

    int connListCurrentIndex() const;
    QString connListCurrentPath() const;

private:
    ConnBlockListModel *m_connBlockListModel = nullptr;

    QPushButton *m_btEdit = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actRemoveConn = nullptr;
    QAction *m_actClearAll = nullptr;
    QToolButton *m_btClearAll = nullptr;
    QPushButton *m_btOptions = nullptr;
    QCheckBox *m_cbAutoScroll = nullptr;
    QCheckBox *m_cbShowHostNames = nullptr;
    TableView *m_connListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // CONNECTIONSPAGE_H
