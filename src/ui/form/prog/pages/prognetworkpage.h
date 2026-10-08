#ifndef PROGNETWORKPAGE_H
#define PROGNETWORKPAGE_H

#include <util/conf/filterlinetext.h>

#include "progbasepage.h"

class Conn;
class ListView;
class RuleSelector;
class StringListModel;
class TerminatingRuleSelector;
class ZonesSelector;

class ProgNetworkPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgNetworkPage(ProgramEditController *ctrl = nullptr, QWidget *parent = nullptr);

    // Adds the connection's filter
    void openConnFilterForm(const Conn &conn);

public slots:
    void fillApp(App &app) const override;

protected slots:
    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void initializeRuleField(bool isSingleSelection);
    void initializeSpeedLimitFields();
    void initializeFilters(bool isSingleSelection);

    void retranslateSpeedLimitFields();
    void retranslateFilters();

    void setupUi();
    QLayout *setupZonesRuleLayout();
    QLayout *setupSpeedLimitsLayout();
    void setupSpeedLimitsChanged();
    QLayout *setupFiltersHeaderLayout();
    void setupFilterListView();
    void setupFilterListViewChanged();

    void updateSpeedLimitCombos();

    int filterListCurrentIndex() const;

    void openFilterEditForm(const FilterLineText &lineText = {}, int row = -1);
    void editCurrentFilter();
    void moveCurrentFilter(int offset);
    void saveFilter(const QString &filterText, int row);

private:
    StringListModel *m_filterListModel = nullptr;

    QLabel *m_labelBlock = nullptr;
    QCheckBox *m_cbBlockInbound = nullptr;
    QCheckBox *m_cbLanOnly = nullptr;
    ZonesSelector *m_btZones = nullptr;
    RuleSelector *m_ruleSelector = nullptr;
    QCheckBox *m_cbSpeedLimitIn = nullptr;
    QComboBox *m_comboSpeedLimitIn = nullptr;
    QCheckBox *m_cbSpeedLimitOut = nullptr;
    QComboBox *m_comboSpeedLimitOut = nullptr;
    QToolButton *m_btAddFilter = nullptr;
    QToolButton *m_btRemoveFilter = nullptr;
    QToolButton *m_btEditFilter = nullptr;
    QToolButton *m_btUpFilter = nullptr;
    QToolButton *m_btDownFilter = nullptr;
    ListView *m_filterListView = nullptr;
    TerminatingRuleSelector *m_terminatingRuleSelector = nullptr;
};

#endif // PROGNETWORKPAGE_H
