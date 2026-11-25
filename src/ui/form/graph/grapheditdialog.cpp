#include "grapheditdialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <user/inigraph.h>
#include <user/usersettings.h>
#include <util/guiutil.h>

#include "graphoptionsarea.h"
#include "graphscontroller.h"

using namespace Fort;

GraphEditDialog::GraphEditDialog(GraphsController *ctrl, int graphId, QWidget *parent) :
    QDialog(parent), m_ctrl(ctrl), m_iniUser(userSettings())
{
    setupUi(graphId);
    setupController();

    retranslateUi();

    m_editName->setStartText(iniGraph().name());
    m_editName->setFocus();
}

IniGraph &GraphEditDialog::iniGraph() const
{
    return m_graphOptions->iniGraph();
}

void GraphEditDialog::retranslateUi()
{
    this->unsetLocale();

    m_labelName->setText(tr("Name:"));

    m_graphOptions->retranslateUi();

    m_btDefault->setText(tr("Default"));
    m_btOk->setText(tr("OK"));
    m_btApply->setText(tr("Apply"));
    m_btCancel->setText(tr("Cancel"));

    this->setWindowTitle(tr("Edit Graph"));
}

void GraphEditDialog::setupController()
{
    connect(ctrl(), &GraphsController::retranslateUi, this, &GraphEditDialog::retranslateUi);
}

void GraphEditDialog::setupUi(int graphId)
{
    // Name
    auto nameLayout = setupNameLayout();

    // Graph's Options
    m_graphOptions = new GraphOptionsArea(IniGraph(m_iniUser, graphId));

    connect(m_graphOptions, &GraphOptionsArea::edited, this, [&] { setEdited(); });

    // Default/OK/Cancel
    auto buttonsLayout = setupButtons();

    auto layout = new QVBoxLayout();
    layout->addLayout(nameLayout);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addWidget(m_graphOptions, 1);
    layout->addWidget(ControlUtil::createHSeparator());
    layout->addLayout(buttonsLayout);

    this->setLayout(layout);

    // Font
    this->setFont(WindowManager::defaultFont());

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/action_log.png"));

    // Help
    ControlUtil::setupHelpShortcut(this, "traffic-graph");

    // Size Grip
    this->setSizeGripEnabled(true);
}

QLayout *GraphEditDialog::setupNameLayout()
{
    auto layout = new QFormLayout();

    m_editName = new LineEdit();
    m_editName->setObjectName("editName");
    m_editName->setMaxLength(256);

    connect(m_editName, &QLineEdit::textEdited, this, [&] { setEdited(); });

    layout->addRow("Name:", m_editName);
    m_labelName = ControlUtil::formRowLabel(layout, m_editName);

    return layout;
}

QLayout *GraphEditDialog::setupButtons()
{
    // Default
    m_btDefault = ControlUtil::createButton(QString(), [&] { m_graphOptions->resetToDefault(); });
    m_btDefault->setObjectName("btDefault");

    // OK
    m_btOk = ControlUtil::createButton(QString(), [&] {
        save();
        this->close();
    });
    m_btOk->setObjectName("btOk");
    m_btOk->setDefault(true);

    // Apply
    m_btApply = ControlUtil::createButton(QString(), [&] {
        save();
        setEdited(false);
    });
    m_btApply->setObjectName("btApply");
    m_btApply->setEnabled(false);

    // Cancel
    m_btCancel = new QPushButton();
    m_btCancel->setObjectName("btCancel");
    connect(m_btCancel, &QAbstractButton::clicked, this, &QWidget::close);

    auto layout = new QHBoxLayout();
    layout->addWidget(m_btDefault);
    layout->addWidget(m_btOk, 1, Qt::AlignRight);
    layout->addWidget(m_btApply);
    layout->addWidget(m_btCancel);

    return layout;
}

void GraphEditDialog::setEdited(bool edited)
{
    m_btApply->setEnabled(edited);
}

void GraphEditDialog::save()
{
    auto &ini = iniGraph();

    ini.setName(m_editName->text());

    ctrl()->saveGraph(m_iniUser, ini.graphId());
}
