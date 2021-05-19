#ifndef CONNECTIONSPAGE_H
#define CONNECTIONSPAGE_H

#include "statbasepage.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class AppInfoCache;
class AppInfoRow;
class ConfManager;
class ConnListModel;
class FirewallConf;
class FortManager;
class FortSettings;
class IniOptions;
class IniUser;
class StatisticsController;
class TableView;

class ConnectionsPage : public StatBasePage
{
    Q_OBJECT

public:
    explicit ConnectionsPage(StatisticsController *ctrl = nullptr, QWidget *parent = nullptr);

    ConnListModel *connListModel() const;
    AppInfoCache *appInfoCache() const;

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupLogOptions();
    void setupAutoScroll();
    void setupShowHostNames();
    void setupTableConnList();
    void setupTableConnListHeader();
    void setupAppInfoRow();
    void setupTableConnsChanged();

    void syncAutoScroll();
    void syncShowHostNames();

    void deleteConn(int row);

    int connListCurrentIndex() const;
    QString connListCurrentPath() const;

private:
    QPushButton *m_btEdit = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actRemoveConn = nullptr;
    QAction *m_actClearConns = nullptr;
    QPushButton *m_btLogOptions = nullptr;
    QCheckBox *m_cbAutoScroll = nullptr;
    QCheckBox *m_cbShowHostNames = nullptr;
    TableView *m_connListView = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // CONNECTIONSPAGE_H
