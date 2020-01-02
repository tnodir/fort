#include "optionswindow.h"

#include <QCloseEvent>
#include <QKeyEvent>
#include <QVBoxLayout>

#include "optionscontroller.h"
#include "pages/mainpage.h"

OptionsWindow::OptionsWindow(FortManager *fortManager,
                             QWidget *parent) :
    WidgetWindow(parent),
    m_ctrl(new OptionsController(fortManager, this))
{
    ctrl()->initialize();

    setupUi();

    emit ctrl()->retranslateUi();
}

void OptionsWindow::closeEvent(QCloseEvent *event)
{
    if (isVisible()) {
        event->ignore();
        ctrl()->closeWindow();
    }
}

void OptionsWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape
            && event->modifiers() == Qt::NoModifier) {
        ctrl()->closeWindow();
    }
}

void OptionsWindow::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_mainPage = new MainPage(ctrl());
    layout->addWidget(m_mainPage);

    this->setLayout(layout);

    // Title
    this->setWindowTitle(tr("Options"));

    // Font
    this->setFont(QFont("Tahoma", 9));

    // Size
    this->resize(1024, 768);
    this->setMinimumSize(800, 500);
}
