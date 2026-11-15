include(../global.pri)

include(FortFirewallDns-include.pri)

CONFIG -= debug_and_release
CONFIG += staticlib

TARGET = FortFirewallDnsLib
TEMPLATE = lib

SOURCES += \
    dns/dnsanswer.cpp \
    dns/dnsquery.cpp \
    dns/dnsutil.cpp

HEADERS += \
    dns/dnsaddresses.h \
    dns/dnsanswer.h \
    dns/dnsquery.h \
    dns/dnsutil.h

# 3rd party integrations
include(3rdparty/c-ares.pri)
