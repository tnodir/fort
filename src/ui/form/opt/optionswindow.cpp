#include "optionswindow.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "../../util/guiutil.h"
#include "optionscontroller.h"
#include "pages/mainpage.h"

OptionsWindow::OptionsWindow(FortManager *fortManager,
                             QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new OptionsController(fortManager, this))
{
    setupUi();
    setupController();
}

void OptionsWindow::setupController()
{
    ctrl()->initialize();

    connect(ctrl(), &OptionsController::retranslateUi, this, &OptionsWindow::onRetranslateUi);

    emit ctrl()->retranslateUi();
}

void OptionsWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();
        ctrl()->closeWindow();
    }
}

void OptionsWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape
            && event->modifiers() == Qt::NoModifier
            && !event->isAutoRepeat()) {
        ctrl()->closeWindow();
    }
}

void OptionsWindow::onRetranslateUi()
{
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
    this->setWindowIcon(GuiUtil::overlayIcon(":/images/sheild-96.png",
                                             ":/images/cog.png"));

    // Size
    this->resize(1024, 768);
    this->setMinimumSize(800, 500);
}
