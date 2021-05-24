
CONFIG(release, debug|release) {
    CONFIG += force_debug_info
}

BREAKPAD_DIR = $$PWD/../../../3rdparty/breakpad/src
INCLUDEPATH += $$BREAKPAD_DIR

SOURCES += \
    $$PWD/crashhandler.cpp

HEADERS += \
    $$PWD/crashhandler.h

win32 {
    # Generate .pdb file
    #QMAKE_CFLAGS_RELEASE += /Zi
    #QMAKE_LFLAGS_RELEASE += /MAP /debug /opt:ref

    SOURCES += \
        $$BREAKPAD_DIR/client/windows/crash_generation/crash_generation_client.cc \
        $$BREAKPAD_DIR/client/windows/handler/exception_handler.cc \
        $$BREAKPAD_DIR/common/windows/guid_string.cc \
        $$BREAKPAD_DIR/common/windows/string_utils.cc

    HEADERS += \
        $$BREAKPAD_DIR/client/windows/common/ipc_protocol.h \
        $$BREAKPAD_DIR/client/windows/crash_generation/crash_generation_client.h \
        $$BREAKPAD_DIR/client/windows/handler/exception_handler.h \
        $$BREAKPAD_DIR/common/scoped_ptr.h \
        $$BREAKPAD_DIR/common/windows/guid_string.h \
        $$BREAKPAD_DIR/common/windows/string_utils-inl.h \
        $$BREAKPAD_DIR/google_breakpad/common/breakpad_types.h \
        $$BREAKPAD_DIR/google_breakpad/common/minidump_format.h
}
