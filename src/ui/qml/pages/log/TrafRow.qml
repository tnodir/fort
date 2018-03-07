import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import com.fortfirewall 1.0

Row {

    Label {
        width: trafCellWidths[0]
        fontSizeMode: Text.Fit
        text: translationManager.trTrigger
              && dateTime
    }

    Label {
        width: trafCellWidths[1]
        fontSizeMode: Text.Fit
        text: translationManager.trTrigger
              && download
    }

    Label {
        width: trafCellWidths[3]
        fontSizeMode: Text.Fit
        text: translationManager.trTrigger
              && upload
    }

    Label {
        width: trafCellWidths[4]
        fontSizeMode: Text.Fit
        text: translationManager.trTrigger
              && sum
    }
}
