import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    function onSaved() {  // override
    }

    Frame {
        anchors.fill: parent

        Button {
            text: translationManager.dummyBool
                  && qsTranslate("qml", "Download Tasix addresses")
            onClicked: {
            }
        }
    }
}
