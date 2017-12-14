import QtQuick 2.9
import QtQuick.Controls 2.2

ScrollBar {

    policy: position ? ScrollBar.AlwaysOn
                     : ScrollBar.AsNeeded
}
