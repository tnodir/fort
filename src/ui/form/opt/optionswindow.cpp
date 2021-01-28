#include "optionswindow.h"

#include <QKeyEvent>
#include <QVBoxLayout>

#include "../../util/guiutil.h"
#include "optionscontroller.h"
#include "pages/mainpage.h"

OptionsWindow::OptionsWindow(FortManager *fortManager, QWidget *parent) :
    WidgetWindow(parent), m_ctrl(new OptionsController(fortManager, this))
{
    setupUi();
    setupController();
}

void OptionsWindow::setupController()
{
    ctrl()->initialize();

    connect(this, &OptionsWindow::aboutToClose, ctrl(), &OptionsController::closeWindow);

    connect(ctrl(), &OptionsController::retranslateUi, this, &OptionsWindow::onRetranslateUi);

    emit ctrl()->retranslateUi();
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

    this->setWindowTitle(tr("Options"));
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
    this->resize(1024, 768);
    this->setMinimumSize(800, 500);
}
