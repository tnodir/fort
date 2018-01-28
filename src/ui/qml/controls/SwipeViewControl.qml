import QtQuick 2.9
import QtQuick.Controls 2.2

SwipeView {

    Binding {
        target: contentItem
        property: "highlightMoveDuration"
        value: interactive ? 250 : 0
    }
}
