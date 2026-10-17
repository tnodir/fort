#ifndef APPPENDINGCONNSWINDOW_H
#define APPPENDINGCONNSWINDOW_H

#include <stat/pendingconnmanager.h>

#include "appconnswindow.h"

QT_FORWARD_DECLARE_CLASS(QToolButton)

class AppPendingConnSearchModel;

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

    void setupUi();
    void setupDecideButtons();
    void setupConnsChanged();

    void updateDecideButtons();

    void decideConns(PendingConnManager::ConnDecision decision);

    QVector<qint64> selectedConnIds(bool outboundOnly = false) const;

private:
    QToolButton *m_btAllow = nullptr;
    QToolButton *m_btBlock = nullptr;
    QToolButton *m_btContinue = nullptr;
    QToolButton *m_btAddFilter = nullptr;
};

#endif // APPPENDINGCONNSWINDOW_H
