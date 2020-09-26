include($$PWD/GoogleTest-include.pri)

HEADERS += \
    $$PWD/googletest.h

SOURCES += \
    $$GTEST_PATH/src/gtest-all.cc \
    $$GMOCK_PATH/src/gmock-all.cc
