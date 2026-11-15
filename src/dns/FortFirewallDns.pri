
include($$PWD/FortFirewallDns-include.pri)

# Link to a static library
LIBS *= -L$$builddir/dns
LIBS *= -lFortFirewallDnsLib
PRE_TARGETDEPS *= $$builddir/dns/FortFirewallDnsLib.lib
