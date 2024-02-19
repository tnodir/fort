#ifndef OPTMAINPAGE_H
#define OPTMAINPAGE_H

#include <QVarLengthArray>

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
    QLayout *setupDialogButtons();
    void setupBackup();
    void setupDefault();
    void setupApplyCancelButtons();

    OptBasePage *currentPage() const;

private:
    QTabWidget *m_tabWidget = nullptr;

    QPushButton *m_btMenu = nullptr;

    QPushButton *m_btBackup = nullptr;
    QAction *m_actExport = nullptr;
    QAction *m_actImport = nullptr;
    QPushButton *m_btDefault = nullptr;
    QAction *m_actDefaultAll = nullptr;
    QAction *m_actDefaultTab = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btApply = nullptr;
    QPushButton *m_btCancel = nullptr;

    QVarLengthArray<OptBasePage *, 7> m_pages;
};

#endif // OPTMAINPAGE_H
