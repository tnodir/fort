TEMPLATE = subdirs

SUBDIRS += \
    driver \
    tests \
    ui \
    ui-bin

driver.file = driver/FortFirewallDriver.pro

ui.depends = driver
ui.file = ui/FortFirewallUI.pro

ui-bin.depends = ui
ui-bin.file = ui/bin/FortFirewallUIBin.pro

tests.depends = ui
tests.file = tests/FortFirewallTests.pro
