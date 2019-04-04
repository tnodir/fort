import QtQuick 2.12
import QtQuick.Controls 2.12

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
