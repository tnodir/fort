#ifndef PROGCONNLISTPAGE_H
#define PROGCONNLISTPAGE_H

#include "progbasepage.h"

class AppConnListModel;
class AppInfoRow;
class TableView;

struct ConnRow;

class ProgConnListPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgConnListPage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

    AppConnListModel *appConnListModel() const { return m_appConnListModel; }

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void setupConnListModel(const App &app);

    void setupUi();
    void setupConnListView();
    void setupConnListMenu();
    void setupConnListHeader();
    void setupAppInfoRow();
    void setupTableConnListChanged();

    int connListCurrentIndex() const;
    const ConnRow &connListCurrentRow() const;
    QString connListCurrentPath() const;

private:
    AppConnListModel *m_appConnListModel = nullptr;

    TableView *m_connListView = nullptr;
    QAction *m_actCopyAsFilter = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actLookupIp = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;
};

#endif // PROGCONNLISTPAGE_H
