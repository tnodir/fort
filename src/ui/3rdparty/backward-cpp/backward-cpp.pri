
BACKWARD_CPP_DIR = $$PWD/../../../3rdparty/backward-cpp
INCLUDEPATH += $$BACKWARD_CPP_DIR

HEADERS += \
    $$BACKWARD_CPP_DIR/backward.hpp

DEFINES += USE_BACKWARD_CPP
