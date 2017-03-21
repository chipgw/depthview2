import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Steam Controller Settings"

    /* Properties for C++ to read and store/load. */
    property QtObject settings: QtObject {
    }

    function reset() {
    }
    function apply() {
    }

    Button {
        text: "Configure in the Steam Overlay"
        onClicked: Plugin.openOverlay()
    }

    /* TODO - Use this. */
}
