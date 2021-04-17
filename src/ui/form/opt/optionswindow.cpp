#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include "../../fortsettings.h"
#include "../../util/guiutil.h"
#include "../../util/window/widgetwindowstatewatcher.h"
#include "optionscontroller.h"
#include "pages/mainpage.h"

OptionsWindow::OptionsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new OptionsController(fortManager, this)),
    m_stateWatcher(new WidgetWindowStateWatcher(this))
{
    setupUi();
    setupController();
    setupStateWatcher();
}

FortSettings *OptionsWindow::settings() const
{
    return ctrl()->settings();
}

void OptionsWindow::saveWindowState()
{
    settings()->setOptWindowGeometry(m_stateWatcher->geometry());
    settings()->setOptWindowMaximized(m_stateWatcher->maximized());

    emit ctrl()->afterSaveWindowState();
}

void OptionsWindow::restoreWindowState()
{
    m_stateWatcher->restore(this, QSize(1024, 768), settings()->optWindowGeometry(),
            settings()->optWindowMaximized());

    emit ctrl()->afterRestoreWindowState();
}

void OptionsWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &OptionsController::editedChanged, this, &QWidget::setWindowModified);
    connect(ctrl(), &OptionsController::retranslateUi, this, &OptionsWindow::onRetranslateUi);

    emit ctrl()->retranslateUi();
}

void OptionsWindow::setupStateWatcher()
{
    m_stateWatcher->install(this);
}

void OptionsWindow::keyPressEvent(QKeyEvent *event)
{
    WidgetWindow::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    switch (event->key()) {
    case Qt::Key_S: // Ctrl+S
        if (event->modifiers() == Qt::ControlModifier) {
            ctrl()->applyChanges();
        }
        break;
    }
}

void OptionsWindow::onRetranslateUi()
{
    this->unsetLocale();

    this->setWindowTitle(tr("Options") + "[*]");
}

void OptionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_mainPage = new MainPage(ctrl());
    layout->addWidget(m_mainPage);

    this->setLayout(layout);

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Icon
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png", ":/icons/cog.png"));

    // Size
    this->setMinimumSize(800, 500);
}
