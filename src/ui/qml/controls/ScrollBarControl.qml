import QtQuick 2.11
import QtQuick.Controls 2.4

ScrollBar {

    policy: position ? ScrollBar.AlwaysOn
                     : ScrollBar.AsNeeded
}
