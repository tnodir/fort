
MOC_DIR = .moc
OBJECTS_DIR = .obj
RCC_DIR = .rcc
UI_DIR = .ui

# Qt Custom Defines
versionAtLeast(QT_VERSION, 6.6.0) {
    DEFINES += QT_NO_AS_CONST
}

# Version
include($$PWD/version/FortFirewallVersion.pri)
