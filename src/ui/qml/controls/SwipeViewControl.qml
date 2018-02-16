import QtQuick 2.11
import QtQuick.Controls 2.4

SwipeView {

    Binding {
        target: contentItem
        property: "highlightMoveDuration"
        value: interactive ? 250 : 0
    }
}
