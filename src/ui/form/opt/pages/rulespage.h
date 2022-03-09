#ifndef RULESPAGE_H
#define RULESPAGE_H

#include "optbasepage.h"

class PolicyListModel;
class TableView;

class RulesPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit RulesPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    PolicyListModel *presetLibListModel() const { return m_presetLibListModel; }
    PolicyListModel *presetAppListModel() const { return m_presetAppListModel; }
    PolicyListModel *globalPreListModel() const { return m_globalPreListModel; }
    PolicyListModel *globalPostListModel() const { return m_globalPostListModel; }

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    void setupPresetSplitter();
    void setupPresetLibBox();
    void setupPresetLibView();
    void setupPresetAppBox();
    void setupPresetAppView();
    void setupGlobalSplitter();
    void setupGlobalPreBox();
    void setupGlobalPreView();
    void setupGlobalPostBox();
    void setupGlobalPostView();

private:
    PolicyListModel *m_presetLibListModel = nullptr;
    PolicyListModel *m_presetAppListModel = nullptr;
    PolicyListModel *m_globalPreListModel = nullptr;
    PolicyListModel *m_globalPostListModel = nullptr;

    QGroupBox *m_gbPresetLib = nullptr;
    QGroupBox *m_gbPresetApp = nullptr;
    QGroupBox *m_gbGlobalPre = nullptr;
    QGroupBox *m_gbGlobalPost = nullptr;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_presetSplitter = nullptr;
    QSplitter *m_globalSplitter = nullptr;
    TableView *m_presetLibListView = nullptr;
    TableView *m_presetAppListView = nullptr;
    TableView *m_globalPreListView = nullptr;
    TableView *m_globalPostListView = nullptr;
};

#endif // RULESPAGE_H
