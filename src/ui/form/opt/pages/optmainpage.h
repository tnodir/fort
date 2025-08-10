#ifndef OPTMAINPAGE_H
#define OPTMAINPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class OptMainPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit OptMainPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    void selectTab(int index);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupTabBar();
    QLayout *setupButtonsLayout();
    void setupBackup();
    void setupDefault();
    void setupApplyCancelButtons();

    OptBasePage *currentPage() const;
    OptBasePage *pageAt(int index) const;

private:
    QTabWidget *m_tabWidget = nullptr;

    QPushButton *m_btMenu = nullptr;

    QPushButton *m_btBackup = nullptr;
    QAction *m_actExport = nullptr;
    QAction *m_actImport = nullptr;
    QAction *m_actImportApps = nullptr;
    QPushButton *m_btDefault = nullptr;
    QAction *m_actDefaultAll = nullptr;
    QAction *m_actDefaultTab = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btApply = nullptr;
    QPushButton *m_btCancel = nullptr;
};

#endif // OPTMAINPAGE_H
