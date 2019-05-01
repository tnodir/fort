import QtQuick 2.13
import QtQuick.Controls 2.13

DelayButton {

    signal delayClicked()

    property bool isActivated: false

    onPressed: {
        isActivated = false;
    }
    onActivated: {
        isActivated = true;
    }
    onReleased: {
        if (isActivated) {
            delayClicked();
            checked = false;
        }
    }
}
