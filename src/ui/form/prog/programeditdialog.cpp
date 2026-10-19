#include "programeditdialog.h"

#include <QActionGroup>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHeaderView>
#include <QHideEvent>
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
#include <form/conn/appconnswindow.h>
#include <form/conn/apppendingconnswindow.h>
#include <form/controls/controlutil.h>
#include <form/controls/lineedit.h>
#include <form/controls/plaintextedit.h>
#include <form/controls/selector/zonesselector.h>
#include <form/controls/spincombo.h>
#include <form/controls/tableview.h>
#include <form/controls/toolbutton.h>
#include <form/dialog/dialogutil.h>
#include <form/prog/pages/progmainpage.h>
#include <form/rule/ruleswindow.h>
#include <fortglobal.h>
#include <fortmanager.h>
#include <manager/windowmanager.h>
#include <model/applistmodel.h>
#include <model/appconnsearchmodel.h>
#include <model/rulelistmodel.h>
#include <stat/pendingconnmanager.h>
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

using namespace Fort;

namespace {

inline constexpr Qt::WindowFlags dialogFlags =
        Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint;

}

ProgramEditDialog::ProgramEditDialog(QWidget *parent, Qt::WindowFlags f) :
    FormWindow(parent, (f == Qt::Widget ? dialogFlags : f)), m_ctrl(new ProgramEditController(this))
{
    setupUi();
    setupController();
}

void ProgramEditDialog::initialize(const App &app, const QVector<qint64> &appIdList)
{
    ctrl()->initialize(app, appIdList);

    updatePendingConnsCount();

    updateConnsWindow(m_connsWindow);
    updateConnsWindow(m_pendingConnsWindow);
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

    connect(pendingConnManager(), &StatConnManager::connChanged, this,
            &ProgramEditDialog::updatePendingConnsCount);
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
    this->setMinimumWidth(550);
}

void ProgramEditDialog::setupMainLayout()
{
    auto layout = ControlUtil::createVLayout();

    m_mainPage = new ProgMainPage(ctrl());
    layout->addWidget(m_mainPage);

    connect(m_mainPage, &ProgMainPage::pendingConnsWindowToggled, this,
            &ProgramEditDialog::switchPendingConnsWindow);
    connect(m_mainPage, &ProgMainPage::connsWindowToggled, this,
            &ProgramEditDialog::switchConnsWindow);

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

AppConnsWindow *ProgramEditDialog::createConnsWindow()
{
    return new AppConnsWindow(ctrl()->app(), this);
}

AppPendingConnsWindow *ProgramEditDialog::createPendingConnsWindow()
{
    return new AppPendingConnsWindow(ctrl()->app(), this);
}

void ProgramEditDialog::hideEvent(QHideEvent *event)
{
    FormWindow::hideEvent(event);

    // Close the program's Connections windows, but not on minimizing
    if (!event->spontaneous()) {
        closeConnsWindow(m_pendingConnsWindow);
        closeConnsWindow(m_connsWindow);
    }
}

void ProgramEditDialog::switchConnsWindow(bool visible)
{
    if (visible && !m_connsWindow) {
        m_connsWindow = createConnsWindow();
        setupConnsWindow(m_connsWindow, [this] { m_mainPage->setConnsWindowChecked(false); });
    }

    showConnsWindow(m_connsWindow, visible);
}

void ProgramEditDialog::switchPendingConnsWindow(bool visible)
{
    if (visible && !m_pendingConnsWindow) {
        m_pendingConnsWindow = createPendingConnsWindow();
        setupConnsWindow(
                m_pendingConnsWindow, [this] { m_mainPage->setPendingConnsWindowChecked(false); });
    }

    showConnsWindow(m_pendingConnsWindow, visible);
}

void ProgramEditDialog::setupConnsWindow(
        AppConnsWindow *window, const std::function<void()> &onAboutToClose)
{
    window->setAddFilterVisible(true);

    ControlUtil::deleteOnClose(window);

    connect(window, &AppConnsWindow::aboutToClose, m_mainPage, onAboutToClose);
    connect(window, &AppConnsWindow::addFilterRequested, m_mainPage,
            &ProgMainPage::addConnFilterRequested);
}

void ProgramEditDialog::showConnsWindow(AppConnsWindow *window, bool visible)
{
    if (visible) {
        WidgetWindow::showWidget(window);
    } else {
        closeConnsWindow(window);
    }
}

void ProgramEditDialog::closeConnsWindow(AppConnsWindow *window)
{
    if (window) {
        window->close();
    }
}

void ProgramEditDialog::updatePendingConnsCount()
{
    // The single program's pending connections
    const int count =
            ctrl()->isSingleSelection() ? pendingConnManager()->appConnsCount(ctrl()->app()) : 0;

    m_mainPage->setPendingConnsCount(count);
}

void ProgramEditDialog::updateConnsWindow(AppConnsWindow *window)
{
    if (!window)
        return;

    const App &app = ctrl()->app();

    if (!AppConnSearchModel::canFilterApp(app)) {
        window->close();
        return;
    }

    window->setApp(app);
}
