import QtQuick 2.12
import QtQuick.Controls 2.5

SwipeView {

    Binding {
        target: contentItem
        property: "highlightMoveDuration"
        value: interactive ? 250 : 0
    }
}
