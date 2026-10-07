include(../Common/Common.pri)

HEADERS += \
    testconfwalkers.h \
    tst_bitutil.h \
    tst_confutil.h \
    tst_connfilter.h \
    tst_dateutil.h \
    tst_fileutil.h \
    tst_filterline.h \
    tst_formatutil.h \
    tst_ioccontainer.h \
    tst_netutil.h \
    tst_ruletextparser.h \
    tst_stringutil.h \
    tst_timeperiod.h \
    tst_wildmatch.h

SOURCES += \
    tst_main.cpp

# Test Data
RESOURCES += data.qrc
