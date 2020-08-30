isEmpty(GOOGLETEST_DIR): GOOGLETEST_DIR=$$(GOOGLETEST_DIR)

GTEST_PATH = $$GOOGLETEST_DIR/googletest
GMOCK_PATH = $$GOOGLETEST_DIR/googlemock

DEFINES += GTEST_LANG_CXX11

INCLUDEPATH *= \
    $$GTEST_PATH \
    $$GTEST_PATH/include \
    $$GMOCK_PATH \
    $$GMOCK_PATH/include

HEADERS += \
    $$PWD/googletest.h

SOURCES += \
    $$GTEST_PATH/src/gtest-all.cc \
    $$GMOCK_PATH/src/gmock-all.cc
