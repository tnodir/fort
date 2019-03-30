
DEFINES += \
    QCUSTOMPLOT_USE_OPENGL

QCUSTOMPLOT_DIR = $$PWD/../../../3rdparty/qcustomplot
INCLUDEPATH += $$QCUSTOMPLOT_DIR

SOURCES += \
    $$QCUSTOMPLOT_DIR/qcustomplot.cpp

HEADERS += \
    $$QCUSTOMPLOT_DIR/qcustomplot.h
