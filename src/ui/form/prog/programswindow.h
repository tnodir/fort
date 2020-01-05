#ifndef PROGRAMSWINDOW_H
#define PROGRAMSWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QDialog)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QRadioButton)

QT_FORWARD_DECLARE_CLASS(AppInfoCache)
QT_FORWARD_DECLARE_CLASS(AppListModel)
QT_FORWARD_DECLARE_CLASS(CheckSpinCombo)
QT_FORWARD_DECLARE_CLASS(FirewallConf)
QT_FORWARD_DECLARE_CLASS(FortManager)
QT_FORWARD_DECLARE_CLASS(FortSettings)
QT_FORWARD_DECLARE_CLASS(ProgramsController)
QT_FORWARD_DECLARE_CLASS(TableView)

QT_FORWARD_DECLARE_STRUCT(AppRow)

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

    void retranslateAppBlockInHours();

    void setupUi();
    void setupAppEditForm();
    void setupComboAppGroups();
    QLayout *setupHeader();
    void setupLogBlocked();
    void setupTableApps();
    void setupTableAppsHeader();
    void setupAppInfoRow();
    void setupAppInfoVersion();
    void setupTableAppsChanged();
    void updateAppEditForm(const AppRow &appRow);

    int appListCurrentIndex() const;
    QString appListCurrentPath() const;

    ProgramsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    FirewallConf *conf() const;
    AppListModel *appListModel() const { return m_appListModel; }
    AppInfoCache *appInfoCache() const;

private:
    bool m_formAppIsEditing = false;

    ProgramsController *m_ctrl = nullptr;

    AppListModel *m_appListModel = nullptr;

    QPushButton *m_btAddApp = nullptr;
    QPushButton *m_btEditApp = nullptr;
    QPushButton *m_btDeleteApp = nullptr;
    QPushButton *m_btAllowApp = nullptr;
    QPushButton *m_btBlockApp = nullptr;
    QLabel *m_labelEditPath = nullptr;
    QLineEdit *m_editPath = nullptr;
    QPushButton *m_btSelectFile = nullptr;
    QLabel *m_labelAppGroup = nullptr;
    QComboBox *m_comboAppGroup = nullptr;
    QRadioButton *m_rbAllowApp = nullptr;
    QRadioButton *m_rbBlockApp = nullptr;
    CheckSpinCombo *m_cscBlockApp = nullptr;
    QPushButton *m_btEditOk = nullptr;
    QPushButton *m_btEditCancel = nullptr;
    QDialog *m_formAppEdit = nullptr;
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
