lessThan(QT_VERSION, 5.7.1): error(This project requires Qt 5.7.1 or later)

QT += core gui qml widgets

CONFIG += c++11

TARGET = FortFirewall
TEMPLATE = app

SOURCES += \
    main.cpp \
    fortmanager.cpp

HEADERS += \
    fortmanager.h

QML_FILES += \
    qml/*.qml

OTHER_FILES += \
    $${QML_FILES} \
    *.ini

TRANSLATIONS += \
    i18n/i18n_ru.ts

# Compiled translation files
RESOURCES += fort_i18n.qrc

# Default FortFirewall.ini
RESOURCES += fort_ini.qrc

# Windows RC
RC_FILE = fort.rc

