include(../common/Test.pri)

SOURCES += \
    $$UIPATH/conf/addressgroup.cpp \
    $$UIPATH/task/tasktasix.cpp \
    $$UIPATH/task/taskworker.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/net/ip4range.cpp \
    $$UIPATH/util/net/netdownloader.cpp \
    $$UIPATH/util/net/netutil.cpp

HEADERS += \
    $$UIPATH/conf/addressgroup.h \
    $$UIPATH/task/tasktasix.h \
    $$UIPATH/task/taskworker.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/net/ip4range.h \
    $$UIPATH/util/net/netdownloader.h \
    $$UIPATH/util/net/netutil.h

DEFINES += \
    PWD='"$$PWD"' \
    TASK_TEST
