import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Gamepad Settings"

    property alias gamepadEnable: gamepadPlugin_Enable.checked

    CheckBox {
        id: gamepadPlugin_Enable
        text: "Enable Gamepad Input"

        checked: true
    }
}
