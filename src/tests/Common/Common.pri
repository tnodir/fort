include($$PWD/../../global.pri)

include($$PWD/../../ui/FortFirewallUI.pri)

QT += testlib

CONFIG += console
CONFIG -= app_bundle debug_and_release

TEMPLATE = app

# GoogleTest
include($$PWD/GoogleTest-include.pri)

# Link to a static library
LIBS *= -L$$builddir/tests/Common
LIBS *= -lCommonLib
PRE_TARGETDEPS *= $$builddir/tests/Common/CommonLib.lib
