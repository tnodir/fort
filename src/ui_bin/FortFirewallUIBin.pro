include(../global.pri)

include(../ui/FortFirewallUI.pri)

CONFIG -= debug_and_release

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp

# Shadow Build: Copy i18n/ to build path
!equals(PWD, $${OUT_PWD}) {
    i18n.files = $$files(../ui/i18n/*.qm)
    i18n.path = $${OUT_PWD}/i18n
    COPIES += i18n
}

# Windows
RC_FILE = FortFirewall.rc
OTHER_FILES += $${RC_FILE}

# 3rd party integrations
include(3rdparty/3rdparty.pri)

# Visual Leak Detector
# 1) On the Project tab, under Build & Run / Build Steps / Details,
# append to the Additional Arguments: CONFIG+=visual_leak_detector
# 2) On the Project tab, under Build & Run / Run / Run Environment,
# append to the PATH: $$VLD_PATH/bin/Win32
visual_leak_detector {
    VLD_PATH = D:/Utils/Dev/VisualLeakDetector

    INCLUDEPATH += $$VLD_PATH/include/
    LIBS += -L"$$VLD_PATH/lib/Win64"
    DEFINES += USE_VISUAL_LEAK_DETECTOR VLD_FORCE_ENABLE
}
