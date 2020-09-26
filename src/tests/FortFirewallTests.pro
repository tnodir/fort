TEMPLATE = subdirs

SUBDIRS = \
    Common \
    LogBufferTest \
    LogReaderTest \
    StatTest \
    UtilTest

LogBufferTest.depends = Common
LogReaderTest.depends = Common
StatTest.depends = Common
UtilTest.depends = Common
