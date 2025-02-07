
MEMPLUMBER_DIR = $$PWD/../../../3rdparty/memplumber
INCLUDEPATH += $$MEMPLUMBER_DIR

SOURCES += \
    $$MEMPLUMBER_DIR/memplumber.cpp \
    $$PWD/memplumberutil.cpp

HEADERS += \
    $$MEMPLUMBER_DIR/memplumber.h \
    $$MEMPLUMBER_DIR/memplumber-internals.h \
    $$PWD/memplumberutil.h
