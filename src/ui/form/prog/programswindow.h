#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(AppListModel)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(ProgramsController)
QT_FORWARD_DECLARE_CLASS(TableView)

class ProgramsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ProgramsWindow(FortManager *fortManager,
                            QWidget *parent = nullptr);

protected slots:
    void onSaveWindowState();
    void onRestoreWindowState();

    void onRetranslateUi();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void setupController();
    void setupAppListModel();

    void setupUi();
    QLayout *setupHeader();
    void setupLogBlocked();
    void setupTableApps();
    void setupTableAppsHeader();
    void setupAppInfoRow();
    void setupAppInfoVersion();

    int appListCurrentIndex() const;
    QString appListCurrentPath() const;

    ProgramsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    FirewallConf *conf() const;
    AppListModel *appListModel() const { return m_appListModel; }
    AppInfoCache *appInfoCache() const;

private:
    ProgramsController *m_ctrl = nullptr;

    AppListModel *m_appListModel = nullptr;

    QPushButton *m_btAllowApp = nullptr;
    QPushButton *m_btBlockApp = nullptr;
    QPushButton *m_btAddApp = nullptr;
    QPushButton *m_btDeleteApp = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    TableView *m_appListView = nullptr;
    QWidget *m_appInfoRow = nullptr;
    QPushButton *m_btAppCopyPath = nullptr;
    QPushButton *m_btAppOpenFolder = nullptr;
    QLabel *m_labelAppPath = nullptr;
    QLabel *m_labelAppProductName = nullptr;
    QLabel *m_labelAppCompanyName = nullptr;
};

#endif // PROGRAMSWINDOW_H
