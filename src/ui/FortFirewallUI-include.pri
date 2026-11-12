
QT += gui network widgets

CONFIG += c++2a

# Driver integration
include($$PWD/../driver/Driver-include.pri)

# DNS integration
include($$PWD/../dns/FortFirewallDns-include.pri)

# 3rd party integrations
include($$PWD/3rdparty/3rdparty-include.pri)

INCLUDEPATH *= $$PWD
