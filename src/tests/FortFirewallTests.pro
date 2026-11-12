TEMPLATE = subdirs

SUBDIRS = \
    Common \
    ConfTest \
    DnsTest \
    LogBufferTest \
    LogReaderTest \
    StatTest \
    UtilTest

ConfTest.depends = Common
DnsTest.depends = Common
LogBufferTest.depends = Common
LogReaderTest.depends = Common
StatTest.depends = Common
UtilTest.depends = Common
