#include "rulesetarea.h"

#include <QToolButton>
#include <QVBoxLayout>

#include <conf/rule.h>
#include <form/controls/controlutil.h>
#include <form/controls/listview.h>
#include <model/rulesetmodel.h>

#include "ruleswindow.h"

RuleSetArea::RuleSetArea(quint8 selectRuleTypes, QWidget *parent) :
    QWidget(parent), m_selectRuleTypes(selectRuleTypes), m_ruleSetModel(new RuleSetModel(this))
{
    setupUi();
}

void RuleSetArea::initialize(const Rule &rule, const QStringList &ruleSetNames)
{
    ruleSetModel()->initialize(rule, ruleSetNames);
}

void RuleSetArea::retranslateUi()
{
    const bool onlyPresets = (m_selectRuleTypes == Rule::ruleTypeBit(Rule::PresetRule));

    m_btAddRule->setText(onlyPresets ? tr("Add Preset Rule") : tr("Add Rule"));
    m_btRemoveRule->setText(tr("Remove"));
    m_btEditRule->setText(tr("Edit"));
    m_btUpRule->setToolTip(tr("Move Up"));
    m_btDownRule->setToolTip(tr("Move Down"));
}

void RuleSetArea::setupUi()
{
    // Header
    auto headerLayout = setupHeaderLayout();

    // RuleSet View
    setupRuleSetView();

    // Actions on rule set view's current changed
    setupRuleSetViewChanged();

    auto layout = ControlUtil::createVLayout(/*margin=*/0);
    layout->addLayout(headerLayout);
    layout->addWidget(m_ruleSetView);

    this->setLayout(layout);
}

QLayout *RuleSetArea::setupHeaderLayout()
{
    m_btAddRule = ControlUtil::createFlatToolButton(":/icons/add.png", [&] { selectRuleDialog(); });
    m_btRemoveRule = ControlUtil::createFlatToolButton(
            ":/icons/delete.png", [&] { ruleSetModel()->remove(ruleSetCurrentIndex()); });
    m_btEditRule = ControlUtil::createFlatToolButton(
            ":/icons/pencil.png", [&] { editCurrentRuleDialog(); });
    m_btUpRule = ControlUtil::createIconToolButton(
            ":/icons/bullet_arrow_up.png", [&] { ruleSetModel()->moveUp(ruleSetCurrentIndex()); });
    m_btDownRule = ControlUtil::createIconToolButton(":/icons/bullet_arrow_down.png",
            [&] { ruleSetModel()->moveDown(ruleSetCurrentIndex()); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_btAddRule, m_btRemoveRule, m_btEditRule,
            ControlUtil::createVSeparator(), m_btUpRule, m_btDownRule, /*stretch*/ nullptr });

    return layout;
}

void RuleSetArea::setupRuleSetView()
{
    m_ruleSetView = new ListView();
    m_ruleSetView->setFlow(QListView::TopToBottom);
    m_ruleSetView->setViewMode(QListView::ListMode);
    m_ruleSetView->setUniformItemSizes(true);
    m_ruleSetView->setAlternatingRowColors(true);

    m_ruleSetView->setModel(ruleSetModel());

    connect(m_ruleSetView, &ListView::doubleClicked, m_btEditRule, &QToolButton::click);
}

void RuleSetArea::setupRuleSetViewChanged()
{
    const auto refreshRuleSetViewChanged = [&] {
        const bool ruleSelected = (ruleSetCurrentIndex() >= 0);
        m_btRemoveRule->setEnabled(ruleSelected);
        m_btEditRule->setEnabled(ruleSelected);
        m_btUpRule->setEnabled(ruleSelected);
        m_btDownRule->setEnabled(ruleSelected);
    };

    refreshRuleSetViewChanged();

    connect(m_ruleSetView, &ListView::currentIndexChanged, this, refreshRuleSetViewChanged);
}

int RuleSetArea::ruleSetCurrentIndex() const
{
    return m_ruleSetView->currentRow();
}

void RuleSetArea::selectRuleDialog()
{
    auto rulesDialog = RulesWindow::showRulesDialog(m_selectRuleTypes, this);

    connect(rulesDialog, &RulesWindow::ruleSelected, ruleSetModel(), &RuleSetModel::addRule);
}

void RuleSetArea::editCurrentRuleDialog()
{
    const int ruleId = ruleSetModel()->ruleSet().value(ruleSetCurrentIndex());

    RulesWindow::showRuleEditDialog(ruleId, this);
}
