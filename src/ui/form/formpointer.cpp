#include "formpointer.h"

#include <form/graph/graphwindow.h>
#include <form/home/homewindow.h>
#include <form/opt/optionswindow.h>
#include <form/prog/programalertwindow.h>
#include <form/prog/programswindow.h>
#include <form/rule/ruleswindow.h>
#include <form/stat/statisticswindow.h>
#include <form/svc/serviceswindow.h>
#include <form/zone/zoneswindow.h>
#include <fortglobal.h>
#include <manager/windowmanager.h>
#include <util/bitutil.h>

using namespace Fort;

namespace {

template<class T>
FormWindow *createWindow()
{
    return new T();
}

using createWindow_func = FormWindow *(*) (void);

static const createWindow_func createWindow_funcList[] = {
    &createWindow<HomeWindow>,
    &createWindow<ProgramsWindow>,
    &createWindow<ProgramAlertWindow>,
    &createWindow<ServicesWindow>,
    &createWindow<OptionsWindow>,
    &createWindow<RulesWindow>,
    &createWindow<StatisticsWindow>,
    &createWindow<ZonesWindow>,
    &createWindow<GraphWindow>,
};

FormWindow *createWindowByCode(WindowCode code)
{
    const int index = BitUtil::bitScanForward(code);

    Q_ASSERT(index >= 0 && index < WindowCount);

    const createWindow_func func = createWindow_funcList[index];

    return func();
}

}

FormPointer::FormPointer(WindowCode code) : m_code(code) { }

FormWindow *FormPointer::initialize()
{
    Q_ASSERT(!m_window);

    auto w = createWindowByCode(code());
    m_window.reset(w);

    QObject::connect(w, &FormWindow::aboutToClose, [&] { close(); });

    return w;
}

bool FormPointer::show(bool activate)
{
    auto windowManager = Fort::windowManager();

    if (!windowManager->checkWindowPassword(code()))
        return false;

    auto w = this->window();

    if (!w) {
        w = initialize();
    }

    w->showWindow(activate);

    windowManager->windowOpened(code());

    return true;
}

bool FormPointer::close()
{
    auto w = this->window();
    if (!w) {
        return false;
    }

    auto windowManager = Fort::windowManager();
    const bool isAppQuitting = windowManager->isAppQuitting();

    if (w->isVisible()) {
        w->saveWindowState(isAppQuitting);
        w->hide();

        windowManager->windowClosed(code());
    }

    if (isAppQuitting || w->deleteOnClose()) {
        emit w->aboutToDelete();
        w->deleteLater();

        m_window.take(); // will be deleted later
        return true;
    }

    return false;
}
