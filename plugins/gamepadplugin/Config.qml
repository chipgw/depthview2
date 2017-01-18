import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Gamepad Settings"

    property bool gamepadEnable: true

    function reset() {
        gamepadPlugin_Enable.checked = gamepadEnable
    }
    function apply() {
        gamepadEnable = gamepadPlugin_Enable.checked
    }

    CheckBox {
        id: gamepadPlugin_Enable
        text: "Enable Gamepad Input"

        checked: gamepadEnable
    }
}
