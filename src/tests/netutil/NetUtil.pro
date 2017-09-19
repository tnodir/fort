include(../common/Test.pri)

SOURCES += \
    $$UIPATH/task/tasktasix.cpp \
    $$UIPATH/util/fileutil.cpp \
    $$UIPATH/util/ip4range.cpp \
    $$UIPATH/util/netutil.cpp

HEADERS += \
    $$UIPATH/task/tasktasix.h \
    $$UIPATH/util/fileutil.h \
    $$UIPATH/util/ip4range.h \
    $$UIPATH/util/netutil.h

DEFINES += PWD='"$$PWD"'
