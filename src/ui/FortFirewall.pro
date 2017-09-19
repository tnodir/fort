QT += core gui qml widgets

CONFIG += c++11

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp \
    activityLog/logbuffer.cpp \
    activityLog/logentry.cpp \
    conf/addressgroup.cpp \
    conf/appgroup.cpp \
    conf/firewallconf.cpp \
    driver/drivermanager.cpp \
    driver/driverworker.cpp \
    fortcommon.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    mainwindow.cpp \
    task/task.cpp \
    task/taskmanager.cpp \
    task/tasktasix.cpp \
    translationmanager.cpp \
    util/confutil.cpp \
    util/device.cpp \
    util/fileutil.cpp \
    util/hostinfo.cpp \
    util/ip4range.cpp \
    util/netutil.cpp \
    util/processinfo.cpp \
    util/osutil.cpp \
    util/stringutil.cpp

HEADERS += \
    activityLog/logbuffer.h \
    activityLog/logentry.h \
    conf/addressgroup.h \
    conf/appgroup.h \
    conf/firewallconf.h \
    driver/drivermanager.h \
    driver/driverworker.h \
    fortcommon.h \
    fortmanager.h \
    fortsettings.h \
    mainwindow.h \
    task/task.h \
    task/taskmanager.h \
    task/tasktasix.h \
    translationmanager.h \
    util/confutil.h \
    util/device.h \
    util/fileutil.h \
    util/hostinfo.h \
    util/ip4range.h \
    util/netutil.h \
    util/processinfo.h \
    util/osutil.h \
    util/stringutil.h

QML_FILES += \
    qml/*.qml \
    qml/controls/*.qml \
    qml/pages/*.qml \
    qml/pages/addresses/*.qml \
    qml/pages/apps/*.qml

OTHER_FILES += \
    $${QML_FILES}

# QML files
RESOURCES += fort_qml.qrc

# Images
RESOURCES += fort_images.qrc

# Shadow Build: Copy i18n/ to build path
!equals(PWD, $${OUT_PWD}) {
    CONFIG(debug, debug|release) {
        OUTDIR = debug
    } else {
        OUTDIR = release
    }

    i18n.files = i18n/*.qm
    i18n.path = $${OUT_PWD}/$${OUTDIR}/i18n
    COPIES += i18n
}

# Windows
LIBS += -lfwpuclnt -lkernel32 -lpsapi -luser32 -luuid -lws2_32
RC_FILE = fort.rc

# Kernel Driver
installer_build {
    BUILDCMD = MSBuild $$PWD/../driver/fortdrv.vcxproj /p:OutDir=./;IntDir=$$OUT_PWD/driver/

    fortdrv32.target = $$PWD/../driver/fortfw32.sys
    fortdrv32.commands = $$BUILDCMD /p:Platform=Win32

    fortdrv64.target = $$PWD/../driver/fortfw64.sys
    fortdrv64.commands = $$BUILDCMD /p:Platform=x64

    QMAKE_EXTRA_TARGETS += fortdrv32 fortdrv64
    PRE_TARGETDEPS += $$fortdrv32.target $$fortdrv64.target
}
