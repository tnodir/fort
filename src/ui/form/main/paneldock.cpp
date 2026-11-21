#include "paneldock.h"

#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>

#include <form/controls/controlutil.h>
#include <form/controls/formwindow.h>
#include <form/controls/windowmenubutton.h>
#include <fortglobal.h>
#include <manager/translationmanager.h>
#include <manager/windowmanager.h>
#include <util/iconcache.h>

#include "mainwindow.h"

using namespace Fort;

PanelDock::PanelDock(FormWindow *formWindow, QWidget *parent) :
    QDockWidget(parent), m_formWindow(formWindow)
{
    setupUi();

    connect(translationManager(), &TranslationManager::languageChanged, this,
            &PanelDock::retranslateUi);
    retranslateUi();

    // By the window's ini group: the Main window's saved layout finds it by the name
    setObjectName(formWindow->iniGroup());

    setWidget(formWindow);

    formWindow->installEventFilter(this);

    updateTitle();
}

void PanelDock::takeFormWindow()
{
    auto w = m_formWindow;

    w->removeEventFilter(this);

    m_formWindow = nullptr;

    // Qt's QWidget::setParent() to a window keeps the old window's focus child when the
    // application has no focus widget: clear it, else the Main window keeps the deleted one
    // and crashes on its next clearFocus()
    if (QWidget *focusWidget = w->focusWidget()) {
        focusWidget->clearFocus();
    }

    setWidget(nullptr);
    w->setParent(nullptr, Qt::Window);
}

bool PanelDock::event(QEvent *event)
{
    // A double click on the title bar doesn't float the dock
    if (event->type() == QEvent::MouseButtonDblClick)
        return true;

    const bool res = QDockWidget::event(event);

    // Dropped where it can't be placed: it stays floating
    if (event->type() == QEvent::MouseButtonRelease && isFloating()) {
        QMetaObject::invokeMethod(this, &PanelDock::onDroppedFloating, Qt::QueuedConnection);
    }

    return res;
}

bool PanelDock::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowTitleChange: {
        updateTitle();
    } break;
    default:
        break;
    }

    return QDockWidget::eventFilter(watched, event);
}

void PanelDock::onDroppedFloating()
{
    if (!isFloating())
        return;

    auto windowManager = Fort::windowManager();

    // Dropped over the Main window: back to its place
    if (windowManager->mainWindow()->frameGeometry().contains(QCursor::pos())) {
        setFloating(false);
        return;
    }

    // Dropped outside the Main window: a window at the drop place
    windowManager->undockWindowByCode(m_formWindow->windowCode(), pos());
}

void PanelDock::retranslateUi()
{
    m_btClose->setToolTip(tr("Close"));
}

void PanelDock::setupUi()
{
    // Floatable while dragged only: a dock dropped outside the Main window becomes a regular window
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    setTitleBarWidget(setupTitleBar());
}

QWidget *PanelDock::setupTitleBar()
{
    m_iconLabel = ControlUtil::createLabel();
    m_iconLabel->setPixmap(IconCache::pixmap(m_formWindow->windowIconPath(), 16));

    m_titleLabel = ControlUtil::createLabel();

    m_btWindowMenu = new WindowMenuButton();
    m_btWindowMenu->setFormWindow(m_formWindow);
    m_btWindowMenu->setDocked(true);

    m_btClose = ControlUtil::createClearButton(
            [&] { windowManager()->closeWindowByCode(m_formWindow->windowCode()); });

    auto layout = ControlUtil::createHLayoutByWidgets(
            { m_iconLabel, m_titleLabel, /*stretch*/ nullptr, m_btWindowMenu, m_btClose });
    layout->setContentsMargins(6, 2, 6, 2);

    auto w = new QWidget();
    w->setLayout(layout);

    w->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(w, &QWidget::customContextMenuRequested, this,
            [=, this](const QPoint &pos) { m_btWindowMenu->menu()->popup(w->mapToGlobal(pos)); });

    return w;
}

void PanelDock::updateTitle()
{
    if (!m_formWindow)
        return;

    const QString title = m_formWindow->windowTitle();

    setWindowTitle(title);

    m_titleLabel->setText(title);
}
