import QtQuick 2.13
import QtQuick.Controls 2.13

SwipeView {

    Binding {
        target: contentItem
        property: "highlightMoveDuration"
        value: interactive ? 250 : 0
    }
}
