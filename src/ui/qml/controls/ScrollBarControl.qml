import QtQuick 2.13
import QtQuick.Controls 2.13

ScrollBar {

    policy: position ? ScrollBar.AlwaysOn
                     : ScrollBar.AsNeeded
}
