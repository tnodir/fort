TEMPLATE = subdirs

SUBDIRS = \
    Common \
    DnsTest \
    LogBufferTest \
    LogReaderTest \
    StatTest \
    UtilTest

DnsTest.depends = Common
LogBufferTest.depends = Common
LogReaderTest.depends = Common
StatTest.depends = Common
UtilTest.depends = Common
