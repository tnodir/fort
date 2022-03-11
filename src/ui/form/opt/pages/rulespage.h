#ifndef RULESPAGE_H
#define RULESPAGE_H

#include "optbasepage.h"

class PolicyListBox;

class RulesPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit RulesPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onSaveWindowState(IniUser *ini) override;
    void onRestoreWindowState(IniUser *ini) override;

    void onRetranslateUi() override;

private:
    void setupUi();
    void setupPresetSplitter();
    void setupPresetLibBox();
    void setupPresetAppBox();
    void setupGlobalSplitter();
    void setupGlobalPreBox();
    void setupGlobalPostBox();

private:
    PolicyListBox *m_presetLibBox = nullptr;
    PolicyListBox *m_presetAppBox = nullptr;
    PolicyListBox *m_globalPreBox = nullptr;
    PolicyListBox *m_globalPostBox = nullptr;
    QSplitter *m_splitter = nullptr;
    QSplitter *m_presetSplitter = nullptr;
    QSplitter *m_globalSplitter = nullptr;
};

#endif // RULESPAGE_H
