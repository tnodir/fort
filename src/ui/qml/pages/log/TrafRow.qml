import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

Row {

    property string dateTime
    property string download
    property string upload
    property string sum

    Label {
        width: trafCellWidths[0]
        fontSizeMode: Text.Fit
        text: translationManager.dummyBool
              && dateTime
    }

    Label {
        width: trafCellWidths[1]
        fontSizeMode: Text.Fit
        text: translationManager.dummyBool
              && download
    }

    Label {
        width: trafCellWidths[3]
        fontSizeMode: Text.Fit
        text: upload
    }

    Label {
        width: trafCellWidths[4]
        fontSizeMode: Text.Fit
        text: sum
    }
}
