#include "proggeneralpage.h"

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/selector/groupsselector.h>
#include <form/controls/spincombo.h>
#include <form/controls/tableview.h>
#include <form/controls/toolbutton.h>
#include <form/dialog/dialogutil.h>
#include <form/prog/programeditcontroller.h>
#include <fortglobal.h>
#include <user/iniuser.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/iconcache.h>
#include <util/net/actionrange.h>
#include <util/stringutil.h>
#include <util/textareautil.h>
#include <util/variantutil.h>

using namespace Fort;

namespace {

const QSize appIconSize(32, 32);

inline constexpr int pathMaxLength = 1024;
inline constexpr int pathMinHeight = 48;
inline constexpr int wildcardMinHeight = 150;
inline constexpr int notesHeight = 40;

const std::array appBlockInMinuteValues = { 15, 0, 1, 5, 10, 30, 60 * 1, 60 * 3, 60 * 6, 60 * 12,
    60 * 24, 60 * 24 * 7, 60 * 24 * 30 };

}

ProgGeneralPage::ProgGeneralPage(ProgramEditController *ctrl, QWidget *parent) :
    ProgBasePage(ctrl, parent)
{
    setupUi();
    setupController();
}

void ProgGeneralPage::selectAction(App::Action action)
{
    m_btgActions->button(int(action))->setChecked(true);
}

void ProgGeneralPage::onPageInitialize(const App &app)
{
    initializePathNameNotesFields(isSingleSelection());

    updateWildcard();

    updateApplyChild();
    m_btGroups->setMask(app.groups);

    selectAction(app.action());

    const bool hasScheduleTime = !app.scheduleTime.isNull();
    const ScheduleType scheduleType = hasScheduleTime ? ScheduleTimeAt : ScheduleTimeIn;

    m_cbSchedule->setChecked(hasScheduleTime);
    m_comboScheduleAction->setCurrentIndex(app.scheduleAction);
    m_comboScheduleType->setCurrentIndex(scheduleType);
    m_scScheduleIn->spinBox()->setValue(5);
    m_dteScheduleAt->setDateTime(app.scheduleTime);
    m_dteScheduleAt->setMinimumDateTime(DateUtil::now());

    initializeFocus();
}

void ProgGeneralPage::initializePathNameNotesFields(bool isSingleSelection)
{
    initializePathField(isSingleSelection);
    initializeNameField(isSingleSelection);
    initializeNotesField(isSingleSelection);
}

void ProgGeneralPage::initializePathField(bool isSingleSelection)
{
    m_editPath->setText(isSingleSelection ? app().appOriginPath : QString());
    m_editPath->setEnabled(isSingleSelection);

    m_btSelectFile->setEnabled(isSingleSelection);

    m_btSwitchWildcard->setChecked(isWildcard());
    m_btSwitchWildcard->setEnabled(isSingleSelection);
}

void ProgGeneralPage::initializeNameField(bool isSingleSelection)
{
    const auto &appName = app().appName;

    m_editName->setStartText(isSingleSelection ? appName : QString());
    m_editName->setEnabled(isSingleSelection);
    m_editName->setClearButtonEnabled(isSingleSelection);

    m_btGetName->setEnabled(isSingleSelection);

    if (isSingleSelection) {
        if (appName.isEmpty()) {
            fillEditName(); // Auto-fill the name
        }
    }
}

void ProgGeneralPage::initializeNotesField(bool isSingleSelection)
{
    m_labelEditNotes->setEnabled(isSingleSelection);

    m_editNotes->setText(app().notes);
    m_editNotes->setEnabled(isSingleSelection);

    m_btSetIcon->setEnabled(isSingleSelection);
    m_btDeleteIcon->setEnabled(isSingleSelection);

    setIconPath(app().iconPath);
}

void ProgGeneralPage::initializeFocus()
{
    if (!isNew()) {
        m_btgActions->checkedButton()->setFocus();
    } else {
        m_editPath->setFocus();
    }
}

void ProgGeneralPage::onRetranslateUi()
{
    m_labelEditPath->setText(isWildcard() ? tr("Wildcard Paths:") : tr("File Path:"));
    retranslatePathPlaceholderText();
    m_btSelectFile->setToolTip(tr("Select File"));
    m_btSwitchWildcard->setToolTip(tr("Switch Wildcard"));

    m_labelEditName->setText(tr("Name:"));
    m_btGetName->setToolTip(tr("Get Program Name"));

    m_editNotes->setPlaceholderText(tr("Notes"));
    m_btSetIcon->setToolTip(tr("Set Icon"));
    m_btDeleteIcon->setToolTip(tr("Delete Icon"));

    m_cbApplyChild->setText(tr("Rules inheritance:"));
    retranslateComboApplyChild();

    m_btGroups->retranslateUi();

    m_rbAllow->setText(ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_ALLOW));
    m_rbBlock->setText(ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_BLOCK));
    m_rbAsk->setText(ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_ASK));
    m_rbAsk->setToolTip(tr("Ask to Connect: pause the connection till the user's decision"));
    m_rbKillProcess->setText(tr("Kill Process"));

    m_cbSchedule->setText(tr("Schedule"));
    retranslateScheduleAction();
    retranslateScheduleType();
    retranslateScheduleIn();
    m_dteScheduleAt->unsetLocale();

    updateQuickAction();
    retranslateTimedMenuActions();
    retranslateTimedAction(m_btTimedAction);
    retranslateTimedAction(m_btTimedRemove);
}

void ProgGeneralPage::retranslatePathPlaceholderText()
{
    if (!(isWildcard() && isSingleSelection())) {
        m_editPath->setPlaceholderText({});
        return;
    }

    const auto placeholderText = tr("# Examples:")
            // Prefix wildcard
            + '\n' + tr("# All programs in the sub-path:")
            + "\nC:\\Git\\**"
            // Name wildcard
            + '\n' + tr("# Name wildcard:")
            + "\nC:\\Store\\app.v*.exe"
              "\nC:\\App.v*\\app.exe"
            // Env var
            + '\n' + tr("# Environment Variable:")
            + "\n%SystemRoot%\\System32\\telnet.exe"
              "\n%ProgramFiles%\\Internet Explorer\\iexplore.exe";

    m_editPath->setPlaceholderText(placeholderText);
}

void ProgGeneralPage::retranslateComboApplyChild()
{
    // Sync with ProgGeneralPage::ApplyChildType
    const QStringList list = {
        tr("Propagate to all child processes"),
        tr("Propagate to designated child processes"),
        tr("Receive from the parent process"),
    };

    ControlUtil::setComboBoxTexts(m_comboApplyChild, list);

    updateApplyChild();
}

void ProgGeneralPage::retranslateScheduleAction()
{
    // By App::Action
    const QStringList list = {
        ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_BLOCK),
        ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_ALLOW),
        tr("Remove"),
        tr("Kill Process"),
        ActionRange::actionTypeText(FORT_RULE_FILTER_ACTION_ASK),
    };

    ControlUtil::setComboBoxTexts(m_comboScheduleAction, list);

    ControlUtil::setComboBoxIcons(m_comboScheduleAction, App::actionIconPaths());
}

void ProgGeneralPage::retranslateScheduleType()
{
    const QStringList list = { tr("In:"), tr("At:") };

    ControlUtil::setComboBoxTexts(m_comboScheduleType, list);
}

void ProgGeneralPage::retranslateScheduleIn()
{
    const QStringList list = {
        tr("Custom"),
        tr("1 second"),
        tr("1 minute"),
        tr("5 minutes"),
        tr("10 minutes"),
        tr("30 minutes"),
        tr("1 hour"),
        tr("3 hours"),
        tr("6 hours"),
        tr("12 hours"),
        tr("Day"),
        tr("Week"),
        tr("Month"),
    };

    m_scScheduleIn->setNames(list);
    m_scScheduleIn->spinBox()->setSuffix(tr(" minute(s)"));
}

void ProgGeneralPage::retranslateTimedMenuActions()
{
    const auto actions = m_timedMenuActions->actions();
    if (actions.isEmpty())
        return;

    const auto names = m_scScheduleIn->names();

    int index = 0;
    for (auto a : actions) {
        const auto name = names[index++];
        a->setText(name);
    }
}

void ProgGeneralPage::retranslateTimedAction(QToolButton *bt)
{
    const int minutes = timedActionMinutes(bt);

    const int index = m_scScheduleIn->getIndexByValue(minutes);
    const QString text =
            (index > 0) ? m_scScheduleIn->names().at(index) : tr("%1 minute(s)").arg(minutes);

    bt->setText(text);
}

void ProgGeneralPage::setupController()
{
    connect(ctrl(), &ProgramEditController::isWildcardSwitched, this,
            &ProgGeneralPage::onWildcardSwitched);

    connect(appInfoCache(), &AppInfoCache::cacheChanged, this, &ProgGeneralPage::updateAppIcon);
}

void ProgGeneralPage::setupUi()
{
    // Form Layout
    auto formLayout = setupFormLayout();

    // Apply Child, Group Layout
    auto applyChildGroupLayout = setupApplyChildGroupLayout();

    // Allow/Block/Kill Actions Layout
    auto actionsLayout = setupActionsLayout();

    setupActionsGroup();

    // Schedule
    auto scheduleLayout = setupScheduleLayout();

    // Main Layout
    auto layout = new QVBoxLayout();
    // The form takes (almost) all the free height against the spacer's stretch 1
    layout->addLayout(formLayout, 100);
    layout->addLayout(applyChildGroupLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addLayout(actionsLayout);
    layout->addWidget(ControlUtil::createSeparator());
    layout->addStretch(1);
    layout->addLayout(scheduleLayout);

    this->setLayout(layout);
}

QLayout *ProgGeneralPage::setupFormLayout()
{
    auto layout = new QFormLayout();
    layout->setHorizontalSpacing(10);

    // Path
    auto pathLayout = setupPathLayout();

    layout->addRow("File Path:", pathLayout);
    m_labelEditPath = ControlUtil::formRowLabel(layout, pathLayout);

    // Name
    auto nameLayout = setupNameLayout();

    layout->addRow("Name:", nameLayout);
    m_labelEditName = ControlUtil::formRowLabel(layout, nameLayout);

    // Notes
    auto notesLayout = setupNotesLayout();

    layout->addRow("Notes:", notesLayout);
    m_labelEditNotes = ControlUtil::formRowLabel(layout, notesLayout);
    m_labelEditNotes->setScaledContents(true);
    m_labelEditNotes->setFixedSize(appIconSize);

    return layout;
}

QLayout *ProgGeneralPage::setupPathLayout()
{
    // Path
    m_editPath = new PlainTextEdit();
    m_editPath->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    // Before it is laid out: a later change isn't seen by a new window's layouts till it is shown,
    // which then enlarges the window to the field's default minimum height
    m_editPath->setMinimumHeight(pathMinHeight);

    // Select File
    m_btSelectFile = ControlUtil::createIconToolButton(":/icons/folder.png", [&] {
        if (!isPathEditable()) {
            AppInfoUtil::openFolder(m_editPath->toPlainText());
            return;
        }

        const auto filePath = DialogUtil::getOpenFileName(
                m_labelEditPath->text(), tr("Programs (*.exe);;All files (*.*)"));

        if (filePath.isEmpty())
            return;

        const auto appPath = FileUtil::toNativeSeparators(filePath);

        if (isWildcard()) {
            TextAreaUtil::appendText(m_editPath, appPath);
        } else {
            m_editPath->setText(appPath);
        }

        fillEditName(); // Auto-fill the name
    });
    m_btSelectFile->setShortcut(QKeyCombination(Qt::CTRL, Qt::Key_O));

    // Switch Wildcard
    m_btSwitchWildcard = ControlUtil::createIconToolButton(
            ":/icons/coding.png", [&] { ctrl()->switchWildcard(); });
    m_btSwitchWildcard->setCheckable(true);

    auto buttonsLayout = ControlUtil::createVLayoutByWidgets(
            { m_btSelectFile, m_btSwitchWildcard, /*stretch*/ nullptr });

    auto layout = new QHBoxLayout();
    layout->addWidget(m_editPath);
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *ProgGeneralPage::setupNameLayout()
{
    m_editName = new LineEdit();
    m_editName->setMaxLength(1024);

    m_btGetName = ControlUtil::createIconToolButton(
            ":/icons/arrow_refresh_small.png", [&] { fillEditName(); });

    auto layout = ControlUtil::createHLayoutByWidgets({ m_editName, m_btGetName });

    return layout;
}

QLayout *ProgGeneralPage::setupNotesLayout()
{
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(notesHeight);

    m_btSetIcon = ControlUtil::createIconToolButton(":/icons/application.png", [&] {
        const auto filePath = DialogUtil::getOpenFileName(
                tr("Icon for program"), tr("Icons (*.ico; *.png; *.exe)"));

        if (filePath.isEmpty())
            return;

        setIconPath(filePath);
    });

    m_btDeleteIcon = ControlUtil::createIconToolButton(
            ":/icons/application_delete.png", [&] { setIconPath({}); });

    auto layout = ControlUtil::createHLayout();
    layout->addWidget(m_editNotes);
    layout->addWidget(m_btSetIcon, 0, Qt::AlignTop);
    layout->addWidget(m_btDeleteIcon, 0, Qt::AlignTop);

    return layout;
}

QLayout *ProgGeneralPage::setupApplyChildGroupLayout()
{
    // Apply Child
    m_comboApplyChild = ControlUtil::createComboBox(
            {}, [&](int /*index*/) { ctrl()->warnRestartNeededOption(); });
    m_comboApplyChild->setMinimumWidth(150);

    setupCbApplyChild();

    // Groups
    m_btGroups = new GroupsSelector();

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_cbApplyChild, m_comboApplyChild, ControlUtil::createVSeparator(), m_btGroups,
                    /*stretch*/ nullptr });

    return layout;
}

void ProgGeneralPage::setupCbApplyChild()
{
    m_cbApplyChild = ControlUtil::createCheckBox(":/icons/document_tree.png");

    const auto refreshApplyChildEnabled = [&](bool checked) {
        m_comboApplyChild->setEnabled(checked);
    };

    refreshApplyChildEnabled(false);

    connect(m_cbApplyChild, &QCheckBox::toggled, this, refreshApplyChildEnabled);
}

QLayout *ProgGeneralPage::setupActionsLayout()
{
    // Allow
    m_rbAllow = new QRadioButton();
    m_rbAllow->setIcon(
            IconCache::icon(ActionRange::actionTypeIconPath(FORT_RULE_FILTER_ACTION_ALLOW)));
    m_rbAllow->setChecked(true);

    // Block
    m_rbBlock = new QRadioButton();
    m_rbBlock->setIcon(
            IconCache::icon(ActionRange::actionTypeIconPath(FORT_RULE_FILTER_ACTION_BLOCK)));

    // Ask
    m_rbAsk = new QRadioButton();
    m_rbAsk->setIcon(IconCache::icon(ActionRange::actionTypeIconPath(FORT_RULE_FILTER_ACTION_ASK)));

    // Kill Process
    m_rbKillProcess = new QRadioButton();
    m_rbKillProcess->setIcon(IconCache::icon(":/icons/scull.png"));

    connect(m_rbKillProcess, &QRadioButton::clicked, ctrl(),
            &ProgramEditController::warnDangerousOption);

    auto layout = ControlUtil::createHLayoutByWidgets(
            { /*stretch*/ nullptr, m_rbAllow, m_rbBlock, m_rbAsk, m_rbKillProcess,
                    /*stretch*/ nullptr });
    layout->setSpacing(20);

    return layout;
}

void ProgGeneralPage::setupActionsGroup()
{
    m_btgActions = new QButtonGroup(this);
    m_btgActions->setExclusive(true);

    m_btgActions->addButton(m_rbAllow, int(App::ActionAllow));
    m_btgActions->addButton(m_rbBlock, int(App::ActionBlock));
    m_btgActions->addButton(m_rbAsk, int(App::ActionAsk));
    m_btgActions->addButton(m_rbKillProcess, int(App::ActionKillProcess));

    connect(m_btgActions, &QButtonGroup::idToggled, this, [&](int id, bool checked) {
        if (!checked)
            return;

        // The asked Program's connections are filtered too
        emit ctrl()->allowToggled(!App::isActionBlocked(App::Action(id)));
    });
}

QLayout *ProgGeneralPage::setupScheduleLayout()
{
    // Schedule Action
    m_comboScheduleAction = ControlUtil::createComboBox();
    m_comboScheduleAction->setMinimumWidth(100);

    // Schedule Type
    setupComboScheduleType();

    // Schedule after N minutes
    m_scScheduleIn = new SpinCombo();
    m_scScheduleIn->spinBox()->setRange(0, 60 * 24 * 30 * 12); // ~Year
    m_scScheduleIn->setValues(appBlockInMinuteValues);

    // Schedule to a specified date & time
    m_dteScheduleAt = new QDateTimeEdit();
    m_dteScheduleAt->setCalendarPopup(true);

    // Quick Actions
    setupQuickAction();
    setupTimedMenu();
    setupTimedAction();
    setupTimedRemove();

    // Schedule Check Box
    setupCbSchedule();

    auto layout = ControlUtil::createHLayoutByWidgets({ m_cbSchedule, m_comboScheduleAction,
            m_comboScheduleType, /*stretch*/ nullptr, m_scScheduleIn, m_dteScheduleAt,
            m_btQuickAction, m_btTimedAction, m_btTimedRemove });

    return layout;
}

void ProgGeneralPage::setupCbSchedule()
{
    m_cbSchedule = ControlUtil::createCheckBox(":/icons/time.png");

    const auto refreshScheduleEnabled = [&](bool checked) {
        m_comboScheduleAction->setVisible(checked);
        m_comboScheduleType->setVisible(checked);

        m_scScheduleIn->setVisible(checked && m_scScheduleIn->isEnabled());
        m_dteScheduleAt->setVisible(checked && m_dteScheduleAt->isEnabled());

        m_btQuickAction->setVisible(!checked);
        m_btTimedAction->setVisible(!checked);
        m_btTimedRemove->setVisible(!checked);
    };

    refreshScheduleEnabled(false);

    connect(m_cbSchedule, &QCheckBox::toggled, this, refreshScheduleEnabled);
}

void ProgGeneralPage::setupComboScheduleType()
{
    m_comboScheduleType = ControlUtil::createComboBox();

    connect(m_comboScheduleType, &QComboBox::currentIndexChanged, this, [&](int index) {
        const bool isTimeIn = (index == ScheduleTimeIn);
        const bool isTimeAt = (index == ScheduleTimeAt);

        m_scScheduleIn->setEnabled(isTimeIn);
        m_dteScheduleAt->setEnabled(isTimeAt);

        const bool checked = m_cbSchedule->isChecked();

        m_scScheduleIn->setVisible(checked && isTimeIn);
        m_dteScheduleAt->setVisible(checked && isTimeAt);
    });
}

void ProgGeneralPage::setupQuickAction()
{
    m_btQuickAction = ControlUtil::createButton({}, [&] {
        selectAction(m_quickActionType);

        ctrl()->saveChanges();
    });

    const auto refreshQuickActionType = [&](bool checked) {
        m_quickActionType = checked ? App::ActionBlock : App::ActionAllow;

        updateQuickAction();
    };

    connect(m_rbAllow, &QRadioButton::toggled, this, refreshQuickActionType);
}

void ProgGeneralPage::setupTimedMenu()
{
    m_timedMenu = ControlUtil::createMenu(this);

    m_timedMenuActions = new QActionGroup(m_timedMenu);

    connect(m_timedMenu, &QMenu::triggered, this, [&](QAction *action) {
        if (!m_timedMenuOwner)
            return;

        const int minutes = action->data().toInt();

        setTimedActionMinutes(m_timedMenuOwner, minutes);
        retranslateTimedAction(m_timedMenuOwner);
    });

    connect(
            m_timedMenu, &QMenu::aboutToHide, this, [&] { m_timedMenuOwner = nullptr; },
            Qt::QueuedConnection);
}

void ProgGeneralPage::setupTimedMenuActions()
{
    if (!m_timedMenuActions->actions().isEmpty())
        return;

    for (int minutes : appBlockInMinuteValues) {
        auto a = m_timedMenu->addAction(QString());
        a->setCheckable(true);
        a->setData(minutes);

        m_timedMenuActions->addAction(a);
    }

    retranslateTimedMenuActions();
}

void ProgGeneralPage::setupTimedAction()
{
    m_btTimedAction = createTimedButton(iniUser().progAlertWindowTimedActionMinutesKey());

    connect(m_btTimedAction, &QToolButton::clicked, this, [&] {
        const auto currentActionType = App::Action(m_btgActions->checkedId());

        selectAction(m_quickActionType);

        const int minutes = timedActionMinutes(m_btTimedAction);

        saveScheduleAction(currentActionType, minutes);
    });
}

void ProgGeneralPage::setupTimedRemove()
{
    m_btTimedRemove = createTimedButton(iniUser().progAlertWindowTimedRemoveMinutesKey());
    m_btTimedRemove->setIcon(IconCache::icon(":/icons/delete.png"));

    connect(m_btTimedRemove, &QToolButton::clicked, this, [&] {
        const int minutes = timedActionMinutes(m_btTimedRemove);

        saveScheduleAction(App::ActionRemove, minutes);
    });
}

QToolButton *ProgGeneralPage::createTimedButton(const QString &iniKey)
{
    auto bt = new ToolButton();
    bt->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    bt->setPopupMode(QToolButton::MenuButtonPopup);
    bt->setMenu(m_timedMenu);

    VariantUtil::setUserData(bt, iniKey);

    connect(bt, &ToolButton::aboutToShowMenu, this, [&] {
        auto bt = qobject_cast<QToolButton *>(sender());

        const int minutes = timedActionMinutes(bt);
        const int index = m_scScheduleIn->getIndexByValue(minutes);

        selectTimedMenuAction(index);

        m_timedMenuOwner = bt;
    });

    return bt;
}

void ProgGeneralPage::updateWildcard()
{
    m_editPath->setReadOnly(!isPathEditable());
    m_editPath->setMinimumHeight(isWildcard() ? wildcardMinHeight : pathMinHeight);

    updateAppIcon();
}

void ProgGeneralPage::updateApplyChild()
{
    const App &app = this->app();

    const ApplyChildType type = app.applyParent ? ApplyChildType::FromParent
            : app.applySpecChild                ? ApplyChildType::ToSpecChild
            : app.applyChild                    ? ApplyChildType::ToChild
                                                : ApplyChildType::Invalid;
    const bool hasApplyChild = (type != ApplyChildType::Invalid);

    m_cbApplyChild->setChecked(hasApplyChild);
    m_comboApplyChild->setCurrentIndex(int(hasApplyChild ? type : ApplyChildType::ToChild));
}

void ProgGeneralPage::updateAppIcon()
{
    QPixmap pixmap;

    if (!m_iconPath.isEmpty()) {
        pixmap = appInfoCache()->fileIcon(m_iconPath).pixmap(appIconSize);
    } else {
        const bool isSingleSelection = m_labelEditNotes->isEnabled();

        pixmap = appIcon(isSingleSelection).pixmap(appIconSize);
    }

    m_labelEditNotes->setPixmap(pixmap);
}

void ProgGeneralPage::updateQuickAction()
{
    auto rb = m_btgActions->button(int(m_quickActionType));

    m_btQuickAction->setIcon(rb->icon());
    m_btQuickAction->setText(rb->text());

    m_btTimedAction->setIcon(rb->icon());
}

void ProgGeneralPage::selectTimedMenuAction(int index)
{
    setupTimedMenuActions();

    auto a = m_timedMenuActions->actions().at(index);
    a->setChecked(true);
}

int ProgGeneralPage::timedActionMinutes(QToolButton *bt)
{
    constexpr int defaultMinutes = 5;

    const auto iniKey = VariantUtil::userData(bt).toString();
    return iniUser().valueInt(iniKey, defaultMinutes);
}

void ProgGeneralPage::setTimedActionMinutes(QToolButton *bt, int minutes)
{
    const auto iniKey = VariantUtil::userData(bt).toString();
    iniUser().setValue(iniKey, minutes);
}

void ProgGeneralPage::fillEditName()
{
    const QString appPath = m_editPath->toPlainText();
    if (appPath.isEmpty())
        return;

    QString appName;
    if (isWildcard()) {
        appName = StringUtil::firstLine(appPath);
    } else {
        const QString normPath = FileUtil::normalizePath(appPath);
        appName = appInfoCache()->appName(normPath);
    }

    m_editName->setStartText(appName);
}

bool ProgGeneralPage::isPathEditable() const
{
    // An existing program's path is read-only, unless it is a wildcard
    return isSingleSelection() && (isWildcard() || isNew() || app().isWildcard);
}

bool ProgGeneralPage::isPathValid() const
{
    if (m_editPath->isEmpty())
        return false;

    if (isWildcard())
        return true;

    const auto path = m_editPath->toPlainText();

    return path.size() <= pathMaxLength && !path.contains('\n');
}

void ProgGeneralPage::onWildcardSwitched()
{
    if (!isWildcard()) {
        m_editPath->setText(isPathEditable() ? StringUtil::firstLine(m_editPath->toPlainText())
                                             : app().appOriginPath);
    }

    updateWildcard();
}

void ProgGeneralPage::setIconPath(const QString &iconPath)
{
    m_iconPath = iconPath;

    const bool hasIconPath = !m_iconPath.isEmpty();

    m_btSetIcon->setVisible(!hasIconPath);
    m_btDeleteIcon->setVisible(hasIconPath);

    updateAppIcon();
}

QIcon ProgGeneralPage::appIcon(bool isSingleSelection) const
{
    if (!isSingleSelection)
        return {};

    if (isWildcard()) {
        return IconCache::icon(":/icons/coding.png");
    }

    return appInfoCache()->appIcon(app().appPath);
}

void ProgGeneralPage::saveScheduleAction(App::Action actionType, int minutes)
{
    m_cbSchedule->setChecked(true);
    m_comboScheduleAction->setCurrentIndex(int(actionType));
    m_comboScheduleType->setCurrentIndex(ScheduleTimeIn);
    m_scScheduleIn->spinBox()->setValue(minutes);

    ctrl()->saveChanges();
}

bool ProgGeneralPage::validateFields() const
{
    // Path
    if (!isPathValid()) {
        m_editPath->setFocus();
        return false;
    }

    // Name
    if (m_editName->text().isEmpty()) {
        m_editName->setFocus();
        return false;
    }

    return true;
}

void ProgGeneralPage::fillApp(App &app) const
{
    app.isWildcard = isWildcard();
    app.setAction(App::Action(m_btgActions->checkedId()));
    app.groups = m_btGroups->mask();
    app.appName = m_editName->text();
    app.notes = m_editNotes->toPlainText();
    app.iconPath = m_iconPath;

    fillAppPath(app);
    fillAppApplyChild(app);
    fillAppEndTime(app);
}

void ProgGeneralPage::fillAppPath(App &app) const
{
    const QString appPath = m_editPath->toPlainText();

    app.appOriginPath = appPath;
    app.appPath = FileUtil::normalizePath(StringUtil::firstLine(appPath));
}

void ProgGeneralPage::fillAppApplyChild(App &app) const
{
    app.applyParent = false;
    app.applyChild = false;
    app.applySpecChild = false;

    if (!m_cbApplyChild->isChecked())
        return;

    const auto type = ApplyChildType(m_comboApplyChild->currentIndex());

    switch (type) {
    case ApplyChildType::ToChild: {
        app.applyChild = true;
    } break;
    case ApplyChildType::ToSpecChild: {
        app.applySpecChild = true;
        app.applyChild = true;
    } break;
    case ApplyChildType::FromParent: {
        app.applyParent = true;
    } break;
    }
}

void ProgGeneralPage::fillAppEndTime(App &app) const
{
    app.scheduleTime = {};

    if (!m_cbSchedule->isChecked()) {
        app.scheduleAction = App::ActionBlock;
        return;
    }

    app.scheduleAction = m_comboScheduleAction->currentIndex();

    switch (m_comboScheduleType->currentIndex()) {
    case ScheduleTimeIn: {
        const int minutes = m_scScheduleIn->spinBox()->value();

        app.scheduleTime = DateUtil::now().addSecs(minutes * 60);
    } break;
    case ScheduleTimeAt: {
        app.scheduleTime = m_dteScheduleAt->dateTime();
    } break;
    }
}
