include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/task/tasktasix.cpp \
    $$UIPATH/task/taskworker.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/ip4range.cpp \
    $$UIPATH/util/netutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/task/tasktasix.h \
    $$UIPATH/task/taskworker.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/ip4range.h \
    $$UIPATH/util/netutil.h

DEFINES += \
    PWD='"$$PWD"' \
    TASK_TEST
