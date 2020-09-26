isEmpty(GOOGLETEST_DIR): GOOGLETEST_DIR=$$(GOOGLETEST_DIR)

!exists($$GOOGLETEST_DIR):message("No GoogleTest source found: set GOOGLETEST_DIR env var.")

GTEST_PATH = $$GOOGLETEST_DIR/googletest
GMOCK_PATH = $$GOOGLETEST_DIR/googlemock

requires(exists($$GTEST_PATH):exists($$GMOCK_PATH))

DEFINES += GTEST_LANG_CXX11

INCLUDEPATH *= \
    $$GTEST_PATH \
    $$GTEST_PATH/include \
    $$GMOCK_PATH \
    $$GMOCK_PATH/include

INCLUDEPATH *= $$PWD
