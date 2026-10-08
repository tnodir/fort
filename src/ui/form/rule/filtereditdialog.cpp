#include "filtereditdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <common/fortconf.h>

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/conf/confbuffer.h>
#include <util/conf/filterline.h>
#include <util/conf/ruletextparser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/textareautil.h>

using namespace Fort;

namespace {

// By the Action button's id: 0 - the Rule's action
const QStringList actionNames = { QString(), "Allow", "Block", "Drop" };

// By the Option check box's index
const QStringList optionNames = { "Log", "NoLog", "Alert" };

int indexOfValue(const QStringList &values, const QString &value)
{
    for (int i = 0, n = values.size(); i < n; ++i) {
        if (values[i].compare(value, Qt::CaseInsensitive) == 0)
            return i;
    }

    return -1;
}

void setComboTexts(QComboBox *c, const QStringList &texts)
{
    for (int i = 0, n = texts.size(); i < n; ++i) {
        c->setItemText(i, texts[i]);
    }
}

bool filterIsNot(const RuleFilter *filter)
{
    return filter && filter->isNot;
}

void addComboFilter(FilterLineText &line, qint8 type, QCheckBox *cbNot, QComboBox *c)
{
    line.addFilter(type, c->currentData().toString(), cbNot->isChecked());
}

void setComboFilter(QCheckBox *cbNot, QComboBox *c, const RuleFilter *filter)
{
    cbNot->setChecked(filterIsNot(filter));

    const int index =
            c->findData(FilterLine::values(filter).value(0), Qt::UserRole, Qt::MatchFixedString);

    c->setCurrentIndex(qMax(index, 0));
}

void addEditFilter(FilterLineText &line, qint8 type, QCheckBox *cbNot, LineEdit *edit)
{
    line.addFilter(type, edit->text().trimmed(), cbNot->isChecked());
}

void setEditFilter(QCheckBox *cbNot, LineEdit *edit, const RuleFilter *filter)
{
    cbNot->setChecked(filterIsNot(filter));
    edit->setStartText(FilterLine::values(filter).join(", "));
}

// The area's lines are the values, the filter's text is a line
void addAreaFilter(FilterLineText &line, qint8 type, QCheckBox *cbNot, PlainTextEdit *area)
{
    QStringList values;

    const QStringList lines = area->toPlainText().split('\n');
    for (const QString &text : lines) {
        const QString value = text.trimmed();
        if (!value.isEmpty()) {
            values << value;
        }
    }

    line.addFilter(type, values.join(", "), cbNot->isChecked());
}

void setAreaFilter(QCheckBox *cbNot, PlainTextEdit *area, const RuleFilter *filter)
{
    cbNot->setChecked(filterIsNot(filter));

    const QSignalBlocker blocker(area); // textChanged() isn't by the user only

    area->setText(FilterLine::values(filter).join('\n'));
}
}

FilterEditDialog::FilterEditDialog(bool isRuleFilter, QWidget *parent) :
    QDialog(parent), m_isRuleFilter(isRuleFilter)
{
    setupUi();
}

void FilterEditDialog::initialize(const FilterLineText &lineText, int row)
{
    m_row = row;

    retranslateUi();

    // The new Program's filter allows
    const bool isNewProgramFilter = lineText.isEmpty() && !isRuleFilter();

    const FilterLineText startText = isNewProgramFilter ? FilterLineText("Act(Allow)") : lineText;

    m_editText->setStartText(startText.text());

    setFilterLine(startText);

    m_editRemoteIps->setFocus();
}

void FilterEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelAction->setText(tr("Action:"));
    m_rbRuleAction->setText(tr("By &Rule"));
    m_rbRuleAction->setToolTip(tr("The Rule's action is applied"));
    m_rbAllow->setText(tr("&Allow"));
    m_rbBlock->setText(tr("&Block"));
    m_rbDrop->setText(tr("&Drop"));
    m_rbDrop->setToolTip(tr("Block silently, without a response"));

    m_labelDirection->setText(tr("Direction:"));
    setComboTexts(m_comboDirection, { tr("Any"), tr("Inbound"), tr("Outbound") });

    m_labelProtocol->setText(tr("Protocol:"));
    setComboTexts(m_comboProtocol, { tr("Any"), "TCP", "UDP", "ICMP", "ICMPv6" });

    m_labelArea->setText(tr("Area:"));
    setComboTexts(m_comboArea, { tr("Any"), tr("Localhost"), "LAN", tr("Internet") });
    m_comboArea->setToolTip(tr("LAN: by the \"Local Area Network\" addresses in Options"));

    retranslateNotCheckBoxes();
    retranslateClearButtons();

    const QString anyText = tr("Any");

    m_labelRemoteIps->setText(tr("Remote IP:"));
    m_editRemoteIps->setPlaceholderText(
            anyText + ":\n1.1.1.1\n10.0.0.0/8\n1.1.1.1-8.8.8.8\n[::1]\n[fe80::]/10");
    m_labelRemotePorts->setText(tr("Remote Port:"));
    m_editRemotePorts->setPlaceholderText(anyText + ": 53, 80-8080");
    m_labelLocalIps->setText(tr("Local IP:"));
    m_editLocalIps->setPlaceholderText(anyText);
    m_labelLocalPorts->setText(tr("Local Port:"));
    m_editLocalPorts->setPlaceholderText(anyText);

    m_labelOptions->setText(tr("Options:"));
    m_cbOptionLog->setText(tr("Log"));
    m_cbOptionLog->setToolTip(tr("Collect the connection, even if the Rule doesn't"));
    m_cbOptionNoLog->setText(tr("No Log"));
    m_cbOptionNoLog->setToolTip(tr("Don't collect the connection"));
    m_cbOptionAlert->setText(tr("Alert"));
    m_cbOptionAlert->setToolTip(
            tr("Collect the connection as alerted, also with \"Alerted only\""));

    m_gbText->setTitle(tr("Text"));
    m_btCopyText->setToolTip(tr("Copy Text"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(isEmpty() ? tr("Add Filter") : tr("Edit Filter"));
}

void FilterEditDialog::retranslateNotCheckBoxes()
{
    const QString text = tr("Not");
    const QString toolTip = tr("All, except these");

    for (QCheckBox *cb : { m_cbDirectionNot, m_cbProtocolNot, m_cbAreaNot, m_cbRemoteIpsNot,
                 m_cbRemotePortsNot, m_cbLocalIpsNot, m_cbLocalPortsNot }) {
        cb->setText(text);
        cb->setToolTip(toolTip);
    }
}

void FilterEditDialog::retranslateClearButtons()
{
    const QString toolTip = tr("Clear");

    for (QToolButton *bt :
            { m_btDirectionClear, m_btProtocolClear, m_btAreaClear, m_btRemoteIpsClear }) {
        bt->setToolTip(toolTip);
    }
}

void FilterEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/filter.png"));

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(450);
}

QLayout *FilterEditDialog::setupMainLayout()
{
    // Form Layout
    auto formLayout = setupFormLayout();

    // Text
    setupTextBox();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(formLayout, 1);
    layout->addWidget(m_gbText);
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *FilterEditDialog::setupFormLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Action
    auto actionsLayout = setupActionsLayout();

    layout->addRow("Action:", actionsLayout);
    m_labelAction = ControlUtil::formRowLabel(layout, actionsLayout);

    layout->addRow(ControlUtil::createHSeparator());

    // Direction
    m_cbDirectionNot = createFieldCheckBox();
    m_comboDirection = createFieldCombo({ QString(), "In", "Out" });
    m_btDirectionClear = createComboClearButton(m_comboDirection);

    m_labelDirection = addWidgetsRow(
            layout, "Direction:", { m_cbDirectionNot, m_comboDirection, m_btDirectionClear });

    // Protocol
    m_cbProtocolNot = createFieldCheckBox();
    m_comboProtocol = createFieldCombo({ QString(), "TCP", "UDP", "ICMP", "ICMPv6" });
    m_btProtocolClear = createComboClearButton(m_comboProtocol);

    m_labelProtocol = addWidgetsRow(
            layout, "Protocol:", { m_cbProtocolNot, m_comboProtocol, m_btProtocolClear });

    layout->addRow(ControlUtil::createHSeparator());

    // Area
    m_cbAreaNot = createFieldCheckBox();
    m_comboArea = createFieldCombo({ QString(), "Localhost", "LAN", "Internet" });
    m_btAreaClear = createComboClearButton(m_comboArea);

    m_labelArea = addWidgetsRow(layout, "Area:", { m_cbAreaNot, m_comboArea, m_btAreaClear });

    // Remote IP
    m_cbRemoteIpsNot = createFieldCheckBox();
    m_btRemoteIpsClear =
            ControlUtil::createClearButton([&] { TextAreaUtil::clearArea(m_editRemoteIps); });
    m_editRemoteIps = createValuesArea();

    m_labelRemoteIps = addAreaRow(layout,
            "Remote IP:", { m_cbRemoteIpsNot, /*stretch*/ nullptr, m_btRemoteIpsClear },
            m_editRemoteIps);

    // Remote Port
    m_cbRemotePortsNot = createFieldCheckBox();
    m_editRemotePorts = createValuesEdit();

    m_labelRemotePorts = addNotRow(layout, "Remote Port:", m_cbRemotePortsNot, m_editRemotePorts);

    layout->addRow(ControlUtil::createHSeparator());

    // Local IP
    m_cbLocalIpsNot = createFieldCheckBox();
    m_editLocalIps = createValuesEdit();

    m_labelLocalIps = addNotRow(layout, "Local IP:", m_cbLocalIpsNot, m_editLocalIps);

    // Local Port
    m_cbLocalPortsNot = createFieldCheckBox();
    m_editLocalPorts = createValuesEdit();

    m_labelLocalPorts = addNotRow(layout, "Local Port:", m_cbLocalPortsNot, m_editLocalPorts);

    layout->addRow(ControlUtil::createHSeparator());

    // Options
    auto optionsLayout = setupOptionsLayout();

    layout->addRow("Options:", optionsLayout);
    m_labelOptions = ControlUtil::formRowLabel(layout, optionsLayout);

    layout->addRow(ControlUtil::createHSeparator());

    return layout;
}

QLayout *FilterEditDialog::setupActionsLayout()
{
    m_buttonGroupAction = new QButtonGroup(this);

    m_rbRuleAction = createActionButton(":/icons/script.png", /*actionId=*/0);
    m_rbRuleAction->setVisible(isRuleFilter());

    m_rbAllow = createActionButton(":/icons/accept.png", /*actionId=*/1);
    m_rbBlock = createActionButton(":/icons/deny.png", /*actionId=*/2);
    m_rbDrop = createActionButton(":/icons/cross.png", /*actionId=*/3);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_rbRuleAction, m_rbAllow, m_rbBlock, m_rbDrop, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

QRadioButton *FilterEditDialog::createActionButton(const QString &iconPath, int actionId)
{
    auto c = new QRadioButton();
    c->setIcon(IconCache::icon(iconPath));

    m_buttonGroupAction->addButton(c, actionId);

    connect(c, &QRadioButton::clicked, this, &FilterEditDialog::updateTextByFields);

    return c;
}

QComboBox *FilterEditDialog::createFieldCombo(const QStringList &values)
{
    auto c = ControlUtil::createComboBox();
    c->setMinimumWidth(150);

    for (const QString &value : values) {
        c->addItem(QString(), value);
    }

    connect(c, &QComboBox::activated, this, &FilterEditDialog::updateTextByFields);

    return c;
}

QCheckBox *FilterEditDialog::createFieldCheckBox()
{
    auto c = new QCheckBox();

    connect(c, &QCheckBox::clicked, this, &FilterEditDialog::updateTextByFields);

    return c;
}

LineEdit *FilterEditDialog::createValuesEdit()
{
    auto c = new LineEdit();
    c->setClearButtonEnabled(true);

    connect(c, &LineEdit::textEdited, this, &FilterEditDialog::updateTextByFields);

    return c;
}

PlainTextEdit *FilterEditDialog::createValuesArea()
{
    auto c = new PlainTextEdit();

    connect(c, &PlainTextEdit::textChanged, this, &FilterEditDialog::updateTextByFields);

    return c;
}

// Selects the combo's "Any" value
QToolButton *FilterEditDialog::createComboClearButton(QComboBox *combo)
{
    return ControlUtil::createClearButton([=, this] {
        combo->setCurrentIndex(0);
        updateTextByFields();
    });
}

// The row's widgets are on the left
QLabel *FilterEditDialog::addWidgetsRow(
        QFormLayout *layout, const QString &labelText, const QList<QWidget *> &widgets)
{
    auto rowLayout = ControlUtil::createHLayoutByWidgets(widgets, /*margin=*/0);
    rowLayout->addStretch();

    layout->addRow(labelText, rowLayout);

    return ControlUtil::formRowLabel(layout, rowLayout);
}

// The area is below the row's widgets, only it grows vertically
QLabel *FilterEditDialog::addAreaRow(QFormLayout *layout, const QString &labelText,
        const QList<QWidget *> &widgets, QWidget *area)
{
    auto rowLayout = ControlUtil::createVLayout();
    rowLayout->addLayout(ControlUtil::createHLayoutByWidgets(widgets, /*margin=*/0));
    rowLayout->addWidget(area);

    layout->addRow(labelText, rowLayout);

    return ControlUtil::formRowLabel(layout, rowLayout);
}

QLabel *FilterEditDialog::addNotRow(
        QFormLayout *layout, const QString &labelText, QCheckBox *cbNot, QWidget *field)
{
    auto rowLayout = ControlUtil::createRowLayout(cbNot, field, /*stretch1=*/0);
    rowLayout->setAlignment(cbNot, Qt::AlignTop);

    layout->addRow(labelText, rowLayout);

    return ControlUtil::formRowLabel(layout, rowLayout);
}

QLayout *FilterEditDialog::setupOptionsLayout()
{
    m_cbOptionLog = createFieldCheckBox();
    m_cbOptionLog->setIcon(IconCache::icon(":/icons/connect.png"));

    m_cbOptionNoLog = createFieldCheckBox();

    m_cbOptionAlert = createFieldCheckBox();
    m_cbOptionAlert->setIcon(IconCache::icon(":/icons/error.png"));

    auto layout =
            ControlUtil::createHLayoutByWidgets({ m_cbOptionLog, m_cbOptionNoLog }, /*margin=*/0);
    layout->addSpacing(10);
    layout->addWidget(m_cbOptionAlert);
    layout->addStretch();

    return layout;
}

void FilterEditDialog::setupTextBox()
{
    // Text
    m_editText = new LineEdit();
    m_editText->setReadOnly(true); // by the fields

    // Copy Text
    m_btCopyText = ControlUtil::createIconToolButton(
            ":/icons/page_copy.png", [&] { GuiUtil::setClipboardData(m_editText->text()); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editText, m_btCopyText });

    m_gbText = new QGroupBox();
    m_gbText->setLayout(layout);
}

QLayout *FilterEditDialog::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            this->close();
        }
    });
    m_btOk->setDefault(true);

    // Cancel
    m_btCancel = new QPushButton();
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btCancel);

    return layout;
}

void FilterEditDialog::updateTextByFields()
{
    m_editText->setStartText(filterText());
}

QString FilterEditDialog::filterText() const
{
    FilterLineText line;

    addComboFilter(line, FORT_RULE_FILTER_TYPE_DIRECTION, m_cbDirectionNot, m_comboDirection);
    addComboFilter(line, FORT_RULE_FILTER_TYPE_PROTOCOL, m_cbProtocolNot, m_comboProtocol);
    addComboFilter(line, FORT_RULE_FILTER_TYPE_AREA, m_cbAreaNot, m_comboArea);
    addAreaFilter(line, FORT_RULE_FILTER_TYPE_ADDRESS, m_cbRemoteIpsNot, m_editRemoteIps);
    addEditFilter(line, FORT_RULE_FILTER_TYPE_PORT, m_cbRemotePortsNot, m_editRemotePorts);
    addEditFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS, m_cbLocalIpsNot, m_editLocalIps);
    addEditFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_PORT, m_cbLocalPortsNot, m_editLocalPorts);
    line.addFilter(FORT_RULE_FILTER_TYPE_OPTION, optionValues());

    // The action is the last: the empty name of the Rule's action
    line.addFilter(
            FORT_RULE_FILTER_TYPE_ACTION, actionNames.value(m_buttonGroupAction->checkedId()));

    return line.text();
}

void FilterEditDialog::setFilterLine(const FilterLineText &lineText)
{
    FilterLine line(lineText);
    line.parse();

    setActionFilter(line.filter(FORT_RULE_FILTER_TYPE_ACTION));

    setComboFilter(
            m_cbDirectionNot, m_comboDirection, line.filter(FORT_RULE_FILTER_TYPE_DIRECTION));
    setComboFilter(m_cbProtocolNot, m_comboProtocol, line.filter(FORT_RULE_FILTER_TYPE_PROTOCOL));
    setComboFilter(m_cbAreaNot, m_comboArea, line.filter(FORT_RULE_FILTER_TYPE_AREA));
    setAreaFilter(m_cbRemoteIpsNot, m_editRemoteIps, line.filter(FORT_RULE_FILTER_TYPE_ADDRESS));
    setEditFilter(m_cbRemotePortsNot, m_editRemotePorts, line.filter(FORT_RULE_FILTER_TYPE_PORT));
    setEditFilter(
            m_cbLocalIpsNot, m_editLocalIps, line.filter(FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS));
    setEditFilter(
            m_cbLocalPortsNot, m_editLocalPorts, line.filter(FORT_RULE_FILTER_TYPE_LOCAL_PORT));

    setOptionFilter(line.filter(FORT_RULE_FILTER_TYPE_OPTION));
}

void FilterEditDialog::setActionFilter(const RuleFilter *filter)
{
    const int actionId = indexOfValue(actionNames, FilterLine::values(filter).value(0));

    QAbstractButton *button = m_buttonGroupAction->button(actionId);
    if (button) {
        button->setChecked(true);
    }
}

QList<QCheckBox *> FilterEditDialog::optionCheckBoxes() const
{
    return { m_cbOptionLog, m_cbOptionNoLog, m_cbOptionAlert };
}

// e.g. "Log, Alert"
QString FilterEditDialog::optionValues() const
{
    QStringList values;

    const auto checkBoxes = optionCheckBoxes();
    for (int i = 0, n = checkBoxes.size(); i < n; ++i) {
        if (checkBoxes[i]->isChecked()) {
            values << optionNames[i];
        }
    }

    return values.join(", ");
}

void FilterEditDialog::setOptionFilter(const RuleFilter *filter)
{
    const QStringList values = FilterLine::values(filter);

    const auto checkBoxes = optionCheckBoxes();
    for (int i = 0, n = checkBoxes.size(); i < n; ++i) {
        checkBoxes[i]->setChecked(values.contains(optionNames[i], Qt::CaseInsensitive));
    }
}

bool FilterEditDialog::save()
{
    if (!validateEditText())
        return false;

    emit filterSaved(m_editText->text().trimmed(), m_row);

    return true;
}

bool FilterEditDialog::validateEditText() const
{
    const QString text = m_editText->text().trimmed();

    if (text.isEmpty()) {
        m_editRemoteIps->setFocus();
        return false;
    }

    ConfBuffer confBuf;
    if (!confBuf.validateRuleText(text)) {
        windowManager()->showErrorBox(confBuf.errorMessage());
        return false;
    }

    return true;
}
