TEMPLATE = subdirs

SUBDIRS = \
    ActivityTest \
    Common \
    ConfTest \
    DnsTest \
    LogBufferTest \
    LogReaderTest \
    StatTest \
    UtilTest

ActivityTest.depends = Common
ConfTest.depends = Common
DnsTest.depends = Common
LogBufferTest.depends = Common
LogReaderTest.depends = Common
StatTest.depends = Common
UtilTest.depends = Common
