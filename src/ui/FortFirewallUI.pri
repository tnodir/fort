
include($$PWD/FortFirewallUI-include.pri)

# Link to a static library
LIBS *= -L$$OUT_PWD/../../ui
LIBS += -lFortFirewallUILib
