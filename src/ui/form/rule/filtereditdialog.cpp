#include "filtereditdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
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
#include <model/connlistmodel.h>
#include <util/conf/confbuffer.h>
#include <util/conf/filterline.h>
#include <util/conf/ruletextparser.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/net/actionrange.h>
#include <util/textareautil.h>

using namespace Fort;

namespace {

// By the Direction check box's index
const QStringList directionNames = { "In", "Out" };

// By the Protocol check box's index
const QStringList protocolNames = { "TCP", "UDP" };

// By the Area check box's index
const QStringList areaNames = { "Localhost", "LAN", "Internet" };

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

// The checked boxes' names by their indexes, e.g. "Log, Alert"
QString checkedNames(const QList<QCheckBox *> &checkBoxes, const QStringList &names)
{
    QStringList values;

    for (int i = 0, n = checkBoxes.size(); i < n; ++i) {
        if (checkBoxes[i]->isChecked()) {
            values << names[i];
        }
    }

    return values.join(", ");
}

// Checks the boxes by their names in the values, returns the other values
QStringList setCheckedNames(
        const QList<QCheckBox *> &checkBoxes, const QStringList &names, QStringList values)
{
    for (int i = 0, n = checkBoxes.size(); i < n; ++i) {
        const int index = indexOfValue(values, names[i]);

        checkBoxes[i]->setChecked(index >= 0);

        if (index >= 0) {
            values.removeAt(index);
        }
    }

    return values;
}

bool filterIsNot(const RuleFilter *filter)
{
    return filter && filter->isNot;
}

void addEditFilter(FilterLineText &line, qint8 type, QToolButton *btNot, LineEdit *edit)
{
    line.addFilter(type, edit->text().trimmed(), btNot->isChecked());
}

void setEditFilter(QToolButton *btNot, LineEdit *edit, const RuleFilter *filter)
{
    btNot->setChecked(filterIsNot(filter));
    edit->setStartText(FilterLine::values(filter).join(", "));
}

// "=Local_IP": the local value equals the remote one, instead of the edit's values
void addEqualFilter(FilterLineText &line, qint8 type, QToolButton *btNot)
{
    line.addFilter(type, QString(), btNot->isChecked(), /*isEqual=*/true);
}

void setEqualEditFilter(
        QToolButton *btNot, QToolButton *btEqual, LineEdit *edit, const RuleFilter *filter)
{
    setEditFilter(btNot, edit, filter);
    btEqual->setChecked(filter && filter->equalValues);
}

// The area's lines are the values, the filter's text is a line
void addAreaFilter(FilterLineText &line, qint8 type, QToolButton *btNot, PlainTextEdit *area)
{
    QStringList values;

    const QStringList lines = area->toPlainText().split('\n');
    for (const QString &text : lines) {
        const QString value = text.trimmed();
        if (!value.isEmpty()) {
            values << value;
        }
    }

    line.addFilter(type, values.join(", "), btNot->isChecked());
}

void setAreaFilter(QToolButton *btNot, PlainTextEdit *area, const RuleFilter *filter)
{
    btNot->setChecked(filterIsNot(filter));

    const QSignalBlocker blocker(area); // textChanged() isn't by the user only

    area->setText(FilterLine::values(filter).join('\n'));
}

void showErrorMessage(const QString &errorMessage)
{
    windowManager()->showErrorBox(errorMessage, FilterEditDialog::tr("Filter Configuration Error"));
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
    m_rbAsk->setText(tr("A&sk"));
    m_rbAsk->setToolTip(tr("Ask to Connect: pause the connection till the user's decision"));
    m_rbDrop->setText(tr("&Drop"));
    m_rbDrop->setToolTip(tr("Block silently, without a response"));

    m_labelDirection->setText(tr("Direction:"));
    m_cbDirectionIn->setText(tr("Inbound"));
    m_cbDirectionOut->setText(tr("Outbound"));

    retranslateNotButtons();

    const QString anyText = tr("Any");

    m_labelProtocols->setText(tr("Protocol:"));

    m_labelAreas->setText(tr("Area:"));
    m_cbAreaLocalhost->setText(tr("Localhost"));
    m_cbAreaLan->setText(tr("LAN"));
    m_cbAreaLan->setToolTip(tr("LAN: by the \"Local Area Network\" addresses in Options"));
    m_cbAreaInet->setText(tr("Internet"));

    m_labelRemoteIps->setText(tr("Remote IP:"));
    m_btRemoteIpsClear->setToolTip(tr("Clear"));
    m_editRemoteIps->setPlaceholderText(
            anyText + ":\n1.1.1.1\n10.0.0.0/8\n1.1.1.1-8.8.8.8\n[::1]\n[fe80::]/10");
    m_labelRemotePorts->setText(tr("Remote Port:"));
    m_editRemotePorts->setPlaceholderText(anyText + ": 53, 80-8080");
    m_labelLocalIps->setText(tr("Local IP:"));
    m_editLocalIps->setPlaceholderText(anyText);
    m_btLocalIpsEqual->setToolTip(tr("Equal to the Remote IP"));
    m_labelLocalPorts->setText(tr("Local Port:"));
    m_editLocalPorts->setPlaceholderText(anyText);
    m_btLocalPortsEqual->setToolTip(tr("Equal to the Remote Port"));

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
    m_btPasteText->setToolTip(tr("Paste Text"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(isEmpty() ? tr("Add Filter") : tr("Edit Filter"));
}

void FilterEditDialog::retranslateNotButtons()
{
    const QString toolTip = tr("All, except these");

    for (QToolButton *bt : { m_btDirectionNot, m_btProtocolsNot, m_btAreasNot, m_btRemoteIpsNot,
                 m_btRemotePortsNot, m_btLocalIpsNot, m_btLocalPortsNot }) {
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
    m_btDirectionNot = createNotButton();

    m_cbDirectionIn = createFieldCheckBox();
    m_cbDirectionIn->setIcon(IconCache::icon(ConnListModel::directionIconPath(/*inbound=*/true)));

    m_cbDirectionOut = createFieldCheckBox();
    m_cbDirectionOut->setIcon(IconCache::icon(ConnListModel::directionIconPath(/*inbound=*/false)));

    m_labelDirection = addWidgetsRow(layout, "Direction:",
            { m_btDirectionNot, m_cbDirectionIn, m_cbDirectionOut, /*stretch*/ nullptr });

    // Protocol
    m_btProtocolsNot = createNotButton();

    m_cbProtocolTcp = createFieldCheckBox();
    m_cbProtocolTcp->setText("TCP");

    m_cbProtocolUdp = createFieldCheckBox();
    m_cbProtocolUdp->setText("UDP");

    m_editProtocols = createValuesEdit();
    m_editProtocols->setPlaceholderText("ICMP, ICMPv6, 47, 50-51");

    m_labelProtocols = addWidgetsRow(layout,
            "Protocol:", { m_btProtocolsNot, m_cbProtocolTcp, m_cbProtocolUdp, m_editProtocols });

    layout->addRow(ControlUtil::createHSeparator());

    // Area
    m_btAreasNot = createNotButton();
    m_cbAreaLocalhost = createFieldCheckBox();
    m_cbAreaLan = createFieldCheckBox();
    m_cbAreaInet = createFieldCheckBox();

    m_labelAreas = addWidgetsRow(layout, "Area:",
            { m_btAreasNot, m_cbAreaLocalhost, m_cbAreaLan, m_cbAreaInet, /*stretch*/ nullptr });

    // Remote IP
    m_btRemoteIpsNot = createNotButton();
    m_btRemoteIpsClear =
            ControlUtil::createClearButton([&] { TextAreaUtil::clearArea(m_editRemoteIps); });
    m_editRemoteIps = createValuesArea();

    m_labelRemoteIps = addAreaRow(layout,
            "Remote IP:", { m_btRemoteIpsNot, /*stretch*/ nullptr, m_btRemoteIpsClear },
            m_editRemoteIps);

    // Remote Port
    m_btRemotePortsNot = createNotButton();
    m_editRemotePorts = createValuesEdit();

    m_labelRemotePorts =
            addWidgetsRow(layout, "Remote Port:", { m_btRemotePortsNot, m_editRemotePorts });

    layout->addRow(ControlUtil::createHSeparator());

    // Local IP
    m_btLocalIpsNot = createNotButton();
    m_editLocalIps = createValuesEdit();
    m_btLocalIpsEqual = createEqualButton(m_editLocalIps);

    m_labelLocalIps = addWidgetsRow(
            layout, "Local IP:", { m_btLocalIpsNot, m_editLocalIps, m_btLocalIpsEqual });

    // Local Port
    m_btLocalPortsNot = createNotButton();
    m_editLocalPorts = createValuesEdit();
    m_btLocalPortsEqual = createEqualButton(m_editLocalPorts);

    m_labelLocalPorts = addWidgetsRow(
            layout, "Local Port:", { m_btLocalPortsNot, m_editLocalPorts, m_btLocalPortsEqual });

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

    // By the button's id: FORT_RULE_FILTER_ACTION_*, 0 - the Rule's action
    m_rbRuleAction = createActionButton(/*actionTypeId=*/0);
    m_rbRuleAction->setIcon(IconCache::icon(":/icons/script.png"));
    m_rbRuleAction->setVisible(isRuleFilter());

    m_rbAllow = createActionButton(FORT_RULE_FILTER_ACTION_ALLOW);
    m_rbBlock = createActionButton(FORT_RULE_FILTER_ACTION_BLOCK);
    m_rbAsk = createActionButton(FORT_RULE_FILTER_ACTION_ASK);
    m_rbDrop = createActionButton(FORT_RULE_FILTER_ACTION_DROP);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_rbRuleAction, m_rbAllow, m_rbBlock, m_rbAsk, m_rbDrop, /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

QRadioButton *FilterEditDialog::createActionButton(int actionTypeId)
{
    auto c = new QRadioButton();
    c->setIcon(IconCache::icon(ActionRange::actionTypeIconPath(actionTypeId)));

    m_buttonGroupAction->addButton(c, actionTypeId);

    connect(c, &QRadioButton::clicked, this, &FilterEditDialog::updateTextByFields);

    return c;
}

QCheckBox *FilterEditDialog::createFieldCheckBox()
{
    auto c = new QCheckBox();

    connect(c, &QCheckBox::clicked, this, &FilterEditDialog::updateTextByFields);

    return c;
}

QToolButton *FilterEditDialog::createNotButton()
{
    auto c = ControlUtil::createFlatToolButton(QString(), [&] { updateTextByFields(); });
    c->setCheckable(true);
    c->setText("=");

    connect(c, &QToolButton::toggled, c,
            [=](bool checked) { c->setText(checked ? QChar(0x2260) /* ≠ */ : QChar('=')); });

    return c;
}

// Replaces the edit's values with "=Local_IP" or "=Local_Port"
QToolButton *FilterEditDialog::createEqualButton(LineEdit *edit)
{
    auto c = ControlUtil::createIconToolButton(
            ":/icons/arrow_join.png", [&] { updateTextByFields(); });
    c->setCheckable(true);

    connect(c, &QToolButton::toggled, edit, &LineEdit::setDisabled);

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

// The nullptr widget is a stretch
QLabel *FilterEditDialog::addWidgetsRow(
        QFormLayout *layout, const QString &labelText, const QList<QWidget *> &widgets)
{
    auto rowLayout = ControlUtil::createHLayoutByWidgets(widgets, /*margin=*/0);
    rowLayout->setSpacing(4);

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

QLayout *FilterEditDialog::setupOptionsLayout()
{
    m_cbOptionLog = createFieldCheckBox();
    m_cbOptionLog->setIcon(IconCache::icon(":/icons/connect.png"));

    m_cbOptionNoLog = createFieldCheckBox();

    m_cbOptionAlert = createFieldCheckBox();
    m_cbOptionAlert->setIcon(IconCache::icon(":/icons/error.png"));

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbOptionLog, m_cbOptionNoLog, m_cbOptionAlert, /*stretch*/ nullptr }, /*margin=*/0);
    layout->setSpacing(4);

    return layout;
}

void FilterEditDialog::setupTextBox()
{
    // Text
    m_editText = new PlainTextEdit();
    m_editText->setReadOnly(true); // by the fields
    m_editText->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_editText->setFixedHeight(60); // 3 lines

    // Copy Text
    m_btCopyText = ControlUtil::createIconToolButton(
            ":/icons/page_copy.png", [&] { GuiUtil::setClipboardData(m_editText->toPlainText()); });

    // Paste Text
    m_btPasteText =
            ControlUtil::createIconToolButton(":/icons/page_paste.png", [&] { pasteText(); });

    auto buttonsLayout = ControlUtil::createVLayoutByWidgets(
            { m_btCopyText, m_btPasteText, /*stretch*/ nullptr }, /*margin=*/0);

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editText });
    layout->addLayout(buttonsLayout);

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
    m_editText->setText(filterText());

    updateClearButtons();
}

void FilterEditDialog::updateClearButtons()
{
    ControlUtil::setVisibleAnimated(m_btRemoteIpsClear, !m_editRemoteIps->toPlainText().isEmpty());
}

QString FilterEditDialog::filterText() const
{
    FilterLineText line;

    line.addFilter(FORT_RULE_FILTER_TYPE_DIRECTION,
            checkedNames(directionCheckBoxes(), directionNames), m_btDirectionNot->isChecked());
    line.addFilter(FORT_RULE_FILTER_TYPE_PROTOCOL, protocolValues(), m_btProtocolsNot->isChecked());
    line.addFilter(FORT_RULE_FILTER_TYPE_AREA, checkedNames(areasCheckBoxes(), areaNames),
            m_btAreasNot->isChecked());
    addAreaFilter(line, FORT_RULE_FILTER_TYPE_ADDRESS, m_btRemoteIpsNot, m_editRemoteIps);
    addEditFilter(line, FORT_RULE_FILTER_TYPE_PORT, m_btRemotePortsNot, m_editRemotePorts);

    if (m_btLocalIpsEqual->isChecked()) {
        addEqualFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS, m_btLocalIpsNot);
    } else {
        addEditFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS, m_btLocalIpsNot, m_editLocalIps);
    }

    if (m_btLocalPortsEqual->isChecked()) {
        addEqualFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_PORT, m_btLocalPortsNot);
    } else {
        addEditFilter(line, FORT_RULE_FILTER_TYPE_LOCAL_PORT, m_btLocalPortsNot, m_editLocalPorts);
    }

    line.addFilter(FORT_RULE_FILTER_TYPE_OPTION, checkedNames(optionCheckBoxes(), optionNames));

    // The action is the last: the empty name of the Rule's action
    line.addFilter(FORT_RULE_FILTER_TYPE_ACTION,
            ActionRange::actionTypeName(quint8(m_buttonGroupAction->checkedId())));

    return line.text();
}

void FilterEditDialog::pasteText()
{
    const QString text = GuiUtil::getClipboardText().trimmed();
    if (text.isEmpty())
        return;

    if (!validateText(text))
        return;

    setFilterLine(text);
}

void FilterEditDialog::setFilterLine(const FilterLineText &lineText)
{
    m_editText->setText(lineText.text());

    FilterLine line(lineText);
    line.parse();

    setActionFilter(line.filter(FORT_RULE_FILTER_TYPE_ACTION));

    setDirectionFilter(line.filter(FORT_RULE_FILTER_TYPE_DIRECTION));
    setProtocolFilter(line.filter(FORT_RULE_FILTER_TYPE_PROTOCOL));
    setAreasFilter(line.filter(FORT_RULE_FILTER_TYPE_AREA));
    setAreaFilter(m_btRemoteIpsNot, m_editRemoteIps, line.filter(FORT_RULE_FILTER_TYPE_ADDRESS));
    setEditFilter(m_btRemotePortsNot, m_editRemotePorts, line.filter(FORT_RULE_FILTER_TYPE_PORT));
    setEqualEditFilter(m_btLocalIpsNot, m_btLocalIpsEqual, m_editLocalIps,
            line.filter(FORT_RULE_FILTER_TYPE_LOCAL_ADDRESS));
    setEqualEditFilter(m_btLocalPortsNot, m_btLocalPortsEqual, m_editLocalPorts,
            line.filter(FORT_RULE_FILTER_TYPE_LOCAL_PORT));

    setOptionFilter(line.filter(FORT_RULE_FILTER_TYPE_OPTION));

    updateClearButtons();
}

void FilterEditDialog::setActionFilter(const RuleFilter *filter)
{
    const quint8 actionTypeId =
            ActionRange::actionTypeIdByName(FilterLine::values(filter).value(0));

    QAbstractButton *button = m_buttonGroupAction->button(actionTypeId);
    if (button) {
        button->setChecked(true);
    }
}

QList<QCheckBox *> FilterEditDialog::directionCheckBoxes() const
{
    return { m_cbDirectionIn, m_cbDirectionOut };
}

void FilterEditDialog::setDirectionFilter(const RuleFilter *filter)
{
    m_btDirectionNot->setChecked(filterIsNot(filter));

    setCheckedNames(directionCheckBoxes(), directionNames, FilterLine::values(filter));
}

QList<QCheckBox *> FilterEditDialog::protocolCheckBoxes() const
{
    return { m_cbProtocolTcp, m_cbProtocolUdp };
}

// The check boxes' protocols and the edit's ones, e.g. "TCP, ICMP"
QString FilterEditDialog::protocolValues() const
{
    QStringList values = { checkedNames(protocolCheckBoxes(), protocolNames),
        m_editProtocols->text().trimmed() };

    values.removeAll(QString());

    return values.join(", ");
}

void FilterEditDialog::setProtocolFilter(const RuleFilter *filter)
{
    m_btProtocolsNot->setChecked(filterIsNot(filter));

    const QStringList otherValues =
            setCheckedNames(protocolCheckBoxes(), protocolNames, FilterLine::values(filter));

    m_editProtocols->setStartText(otherValues.join(", "));
}

QList<QCheckBox *> FilterEditDialog::areasCheckBoxes() const
{
    return { m_cbAreaLocalhost, m_cbAreaLan, m_cbAreaInet };
}

void FilterEditDialog::setAreasFilter(const RuleFilter *filter)
{
    m_btAreasNot->setChecked(filterIsNot(filter));

    setCheckedNames(areasCheckBoxes(), areaNames, FilterLine::values(filter));
}

QList<QCheckBox *> FilterEditDialog::optionCheckBoxes() const
{
    return { m_cbOptionLog, m_cbOptionNoLog, m_cbOptionAlert };
}

void FilterEditDialog::setOptionFilter(const RuleFilter *filter)
{
    setCheckedNames(optionCheckBoxes(), optionNames, FilterLine::values(filter));
}

bool FilterEditDialog::save()
{
    if (!validateEditText())
        return false;

    emit filterSaved(m_editText->toPlainText().trimmed(), m_row);

    return true;
}

bool FilterEditDialog::validateEditText() const
{
    const QString text = m_editText->toPlainText().trimmed();

    if (text.isEmpty()) {
        m_editRemoteIps->setFocus();
        return false;
    }

    return validateText(text);
}

bool FilterEditDialog::validateText(const QString &text) const
{
    // A line of the filters, as copied
    if (text.contains('\n')) {
        showErrorMessage(tr("The text must be one line of filters"));
        return false;
    }

    ConfBuffer confBuf;
    if (!confBuf.validateRuleText(text)) {
        showErrorMessage(confBuf.errorMessage());
        return false;
    }

    return true;
}
