#include "groupeditdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <manager/windowmanager.h>
#include <model/grouplistmodel.h>
#include <util/guiutil.h>

#include "groupscontroller.h"

GroupEditDialog::GroupEditDialog(GroupsController *ctrl, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl)
{
    setupUi();
    setupController();
}

GroupListModel *GroupEditDialog::groupListModel() const
{
    return ctrl()->groupListModel();
}

void GroupEditDialog::initialize(const Group &group)
{
    m_group = group;

    retranslateUi();

    m_editName->setStartText(group.groupName);
    m_cbEnabled->setChecked(group.enabled);

    initializeFocus();
}

void GroupEditDialog::initializeFocus()
{
    m_editName->setFocus();
}

void GroupEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelName->setText(tr("Name:"));

    m_cbEnabled->setText(tr("Enabled"));
    m_cbExclusive->setText(tr("Exclusive"));
    m_editNotes->setPlaceholderText(tr("Notes"));

    m_btOk->setText(tr("OK"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Group"));
}

void GroupEditDialog::setupController()
{
    connect(ctrl(), &GroupsController::retranslateUi, this, &GroupEditDialog::retranslateUi);
}

void GroupEditDialog::setupUi()
{
    // Main Layout
    auto layout = setupMainLayout();
    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/application_double.png"));

    // Size Grip
    this->setSizeGripEnabled(true);

    // Size
    this->setMinimumWidth(500);
}

QLayout *GroupEditDialog::setupMainLayout()
{
    // Name & Sources
    auto nameLayout = setupNameLayout();

    // OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    return layout;
}

QLayout *GroupEditDialog::setupNameLayout()
{
    auto layout = new QFormLayout();

    // Name
    m_editName = new LineEdit();
    m_editName->setMaxLength(256);

    layout->addRow("Name:", m_editName);
    m_labelName = ControlUtil::formRowLabel(layout, m_editName);

    // Enabled
    m_cbEnabled = new QCheckBox();

    layout->addRow(QString(), m_cbEnabled);

    return layout;
}

QLayout *GroupEditDialog::setupNotesLayout()
{
    m_editNotes = new PlainTextEdit();
    m_editNotes->setFixedHeight(40);
}

QLayout *GroupEditDialog::setupButtons()
{
    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        if (save()) {
            emit saved();
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

bool GroupEditDialog::save()
{
    Group group;
    fillGroup(group);

    // Add new group
    if (isEmpty()) {
        return ctrl()->addOrUpdateGroup(group);
    }

    // Edit selected zone
    return saveGroup(group);
}

bool GroupEditDialog::saveGroup(Group &group)
{
    if (!group.isOptionsEqual(m_zone)) {
        group.groupId = m_group.groupId;

        return ctrl()->addOrUpdateGroup(group);
    }

    if (!group.isNameEqual(m_group)) {
        return ctrl()->updateGroupName(m_group.groupId, group.groupName);
    }

    return true;
}

void GroupEditDialog::fillGroup(Group &group) const
{
    group.groupName = m_editName->text();
    group.enabled = m_cbEnabled->isChecked();
    group.exclusive = m_cbExclusive->isChecked();
}
