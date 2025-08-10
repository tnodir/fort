#include "programeditdialog.h"

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>

#include <appinfo/appinfocache.h>
#include <appinfo/appinfoutil.h>
#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/spincombo.h>
#include <form/controls/tableview.h>
#include <form/controls/toolbutton.h>
#include <form/controls/zonesselector.h>
#include <form/dialog/dialogutil.h>
#include <form/prog/pages/progmainpage.h>
#include <form/rule/ruleswindow.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <model/appconnlistmodel.h>
#include <model/applistmodel.h>
#include <model/rulelistmodel.h>
#include <util/dateutil.h>
#include <util/fileutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/ioc/ioccontainer.h>
#include <util/osutil.h>
#include <util/stringutil.h>
#include <util/textareautil.h>
#include <util/variantutil.h>

#include "programeditcontroller.h"

ProgramEditDialog::ProgramEditDialog(QWidget *parent, Qt::WindowFlags f) :
    FormWindow(parent, (f == Qt::Widget ? Qt::Dialog : f)), m_ctrl(new ProgramEditController(this))
{
    setupUi();
    setupController();
}

void ProgramEditDialog::initialize(const App &app, const QVector<qint64> &appIdList)
{
    ctrl()->initialize(app, appIdList);
}

bool ProgramEditDialog::isNew() const
{
    return ctrl()->isNew();
}

void ProgramEditDialog::setupController()
{
    connect(this, &WidgetWindow::defaultKeyPressed, ctrl(), &ProgramEditController::saveChanges);

    connect(ctrl(), &ProgramEditController::closeOnSave, this, [&] { closeOnSave(); });
    connect(ctrl(), &ProgramEditController::closeDialog, this, &QWidget::close);

    connect(ctrl(), &ProgramEditController::retranslateUi, this, &ProgramEditDialog::retranslateUi);
}

void ProgramEditDialog::setupUi()
{
    // Main Layout
    setupMainLayout();

    // Font
    this->setFont(WindowManager::defaultFont());

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/icons/fort.png", ":/icons/application.png"));

    // Modality
    this->setWindowModality(Qt::WindowModal);

    // Size
    this->setMinimumWidth(500);
}

void ProgramEditDialog::setupMainLayout()
{
    auto layout = ControlUtil::createVLayout();

    m_mainPage = new ProgMainPage(ctrl());
    layout->addWidget(m_mainPage);

    this->setLayout(layout);
}

void ProgramEditDialog::retranslateUi()
{
    this->unsetLocale();

    retranslateWindowTitle();
}

void ProgramEditDialog::retranslateWindowTitle()
{
    this->setWindowTitle(ctrl()->isWildcard() ? tr("Edit Wildcard") : tr("Edit Program"));
}
