TEMPLATE = subdirs

SUBDIRS += \
    driver \
    tests \
    ui

driver.file = driver/FortFirewallDriver.pro
tests.file = tests/FortFirewallTests.pro
ui.file = ui/FortFirewallUI.pro
