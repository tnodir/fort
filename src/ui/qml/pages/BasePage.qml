import QtQuick 2.9
import QtQuick.Controls 2.2

Frame {

    function pageFillConf(conf) {
    }

    Connections {
        target: fortManager
        onFillConf: pageFillConf(newConf)
    }
}
