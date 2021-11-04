TEMPLATE = subdirs

SUBDIRS += \
    driver \
    tests \
    ui \
    ui_bin

driver.file = driver/FortFirewallDriver.pro

ui.depends = driver
ui.file = ui/FortFirewallUI.pro

ui_bin.depends = ui
ui_bin.file = ui_bin/FortFirewallUIBin.pro

tests.depends = ui
tests.file = tests/FortFirewallTests.pro

# Driver Payload
driver_payload {
    SUBDIRS += driver_payload

    driver_payload.depends = ui
    driver_payload.file = driver_payload/FortFirewallDriverPayload.pro
}
