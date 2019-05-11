import QtQuick 2.13
import QtQuick.Controls 2.13
import com.fortfirewall 1.0

SpinBoxControl {
    id: field

    from: 0
    to: 24 * 60
    stepSize: 5

    value: defaultValue
    defaultValue: timeToMinutes(defaultTime)

    textFromValue: function(value, locale) {
        return minutesToTime(value);
    }
    valueFromText: function(text, locale) {
        return timeToMinutes(text);
    }

    readonly property string valueTime: minutesToTime(value)
    property string defaultTime

    function timeToMinutes(time) {
        const h = dateUtil.parseTimeHour(time);
        const m = dateUtil.parseTimeMinute(time);

        return h * 60 + m;
    }

    function minutesToTime(v) {
        const h = Math.floor(v / 60);
        const m = v % 60;

        return dateUtil.formatTime(h, m);
    }

    Binding {
        target: contentItem
        property: "inputMask"
        value: "99:99"
    }
}
