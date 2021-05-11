include(../../global.pri)

include(../../ui/FortFirewallUI.pri)

CONFIG += console
CONFIG -= app_bundle debug_and_release

TARGET = CommonLib
CONFIG += staticlib
TEMPLATE = lib

# Mocks
include(mocks/Mocks.pri)

# GoogleTest
include(GoogleTest.pri)
