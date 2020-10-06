include(../Common/Common.pri)

QT += gui

HEADERS += \
    tst_stat.h

SOURCES += \
    tst_main.cpp

# Test Data
RESOURCES += data.qrc

# Stat Migrations
RESOURCES += $$UI_PWD/stat/stat-migrations.qrc
