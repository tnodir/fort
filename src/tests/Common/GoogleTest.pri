include($$PWD/GoogleTest-include.pri)

SOURCES += \
    $$GTEST_PATH/src/gtest-all.cc \
    $$GMOCK_PATH/src/gmock-all.cc

HEADERS += \
    $$PWD/googletest.h
