import QtQuick 2.12
import QtQuick.Controls 2.12

ScrollBar {

    policy: position ? ScrollBar.AlwaysOn
                     : ScrollBar.AsNeeded
}
