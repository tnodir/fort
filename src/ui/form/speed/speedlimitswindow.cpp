#include "speedlimitswindow.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <form/controls/controlutil.h>
#include <form/controls/treeview.h>
#include <form/dialog/dialogutil.h>
#include <manager/windowmanager.h>
// #include <model/speedlimitlistmodel.h>
#include <user/iniuser.h>
#include <util/conf/confutil.h>
#include <util/guiutil.h>
#include <util/iconcache.h>
#include <util/window/widgetwindowstatewatcher.h>

// #include "speedlimiteditdialog.h"
// #include "speedlimitscontroller.h"

namespace {

constexpr int SPEED_LIMITS_VERSION = 1;

}

SpeedLimitsWindow::SpeedLimitsWindow(QWidget *parent) : FormWindow(parent) { }
