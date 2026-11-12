include(../global.pri)

include(FortFirewallDns-include.pri)

CONFIG -= debug_and_release
CONFIG += staticlib

TARGET = FortFirewallDnsLib
TEMPLATE = lib

SOURCES += \
    dns/dnsanswer.cpp \
    dns/dnsclientanswer.cpp \
    dns/dnsetwlistener.cpp \
    dns/dnslistener.cpp \
    dns/dnsquery.cpp \
    dns/dnsresolver.cpp \
    dns/dnsutil.cpp

HEADERS += \
    dns/dnsaddresses.h \
    dns/dnsanswer.h \
    dns/dnsclientanswer.h \
    dns/dnsetwlistener.h \
    dns/dnslistener.h \
    dns/dnsquery.h \
    dns/dnsrequest.h \
    dns/dnsresolver.h \
    dns/dnsutil.h

# 3rd party integrations
include(3rdparty/c-ares.pri)
