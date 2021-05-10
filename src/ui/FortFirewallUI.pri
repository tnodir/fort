
include($$PWD/FortFirewallUI-include.pri)

# Link to a static library
LIBS *= -L$$builddir/ui
LIBS += -lFortFirewallUILib
