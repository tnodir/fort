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
    dns/dnshttpsresolver.cpp \
    dns/dnshttpsserver.cpp \
    dns/dnslistener.cpp \
    dns/dnsplainresolver.cpp \
    dns/dnsquery.cpp \
    dns/dnsresolver.cpp \
    dns/dnsserverstest.cpp \
    dns/dnsutil.cpp

HEADERS += \
    dns/dnsaddresses.h \
    dns/dnsanswer.h \
    dns/dnsclientanswer.h \
    dns/dnsetwlistener.h \
    dns/dnshttpsresolver.h \
    dns/dnshttpsserver.h \
    dns/dnslistener.h \
    dns/dnsplainresolver.h \
    dns/dnsquery.h \
    dns/dnsrequest.h \
    dns/dnsresolver.h \
    dns/dnsserverstest.h \
    dns/dnsutil.h

# 3rd party integrations
include(3rdparty/c-ares.pri)
