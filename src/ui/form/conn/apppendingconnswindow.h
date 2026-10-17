#ifndef APPPENDINGCONNSWINDOW_H
#define APPPENDINGCONNSWINDOW_H

#include <functional>

#include <stat/pendingconnmanager.h>

#include "appconnswindow.h"

QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppPendingConnSearchModel;
struct ConnRow;

/* The program's pending (Ask to Connect) connections, decided by the user once */
class AppPendingConnsWindow : public AppConnsWindow
{
    Q_OBJECT

public:
    explicit AppPendingConnsWindow(const App &app, QWidget *parent = nullptr);

protected:
    AppPendingConnsWindow(const App &app, QWidget *parent, Qt::WindowFlags f);

    AppPendingConnSearchModel *pendingConnListModel() const;

protected slots:
    void retranslateWindowTitle() override;

private:
    void setupController();

    void retranslateUi();
    void retranslateAllowProcessMenu();

    void setupUi();
    void setupDecideButtons();
    void setupAllowProcessButton();
    void setupConnsChanged();

    void updateDecideButtons();

    void decideConns(PendingConnManager::ConnDecision decision);
    void allowProcesses(quint32 seconds);

    QVector<qint64> selectedConnIds(bool outboundOnly = false) const;
    QVector<quint32> selectedProcessIds() const;

    void walkSelectedPendingConns(const std::function<void(const ConnRow &connRow)> &func) const;

private:
    QToolButton *m_btAllow = nullptr;
    QToolButton *m_btAllowProcess = nullptr;
    QToolButton *m_btBlock = nullptr;
    QToolButton *m_btContinue = nullptr;
};

#endif // APPPENDINGCONNSWINDOW_H
