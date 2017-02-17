import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Gamepad Settings"

    /* Properties for C++ to read and store/load. */
    property QtObject settings: QtObject {
        property bool gamepadEnable: true
    }

    function reset() {
        gamepadPlugin_Enable.checked = settings.gamepadEnable
    }
    function apply() {
        settings.gamepadEnable = gamepadPlugin_Enable.checked
    }

    CheckBox {
        id: gamepadPlugin_Enable
        text: "Enable Gamepad Input"
    }
}
