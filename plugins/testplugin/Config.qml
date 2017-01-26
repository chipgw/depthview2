import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Test Plugin Settings"

    /* Properties for C++ to read. */
    property bool logRenderStart: false
    property bool logRenderEnd: false
    property bool logFrameSwap: false
    property bool lockMouse: false
    property real renderSizeFactor: 1.0

    function reset() {
        testPluginLogRenderStart.checked = logRenderStart
        testPluginLogRenderEnd.checked = logRenderEnd
        testPluginLogFrameSwap.checked = logFrameSwap
        testPluginLockMouse.checked = lockMouse
        testPluginRenderSizeFac.value = renderSizeFactor
    }
    function apply() {
        logRenderStart = testPluginLogRenderStart.checked
        logRenderEnd = testPluginLogRenderEnd.checked
        logFrameSwap = testPluginLogFrameSwap.checked
        lockMouse = testPluginLockMouse.checked
        renderSizeFactor = testPluginRenderSizeFac.value
    }

    CheckBox {
        /* IDs need to be unique per plugin. Best way to guarantee that is to prefix with plugin name. */
        id: testPluginLogRenderStart
        text: "Log Render Start"
    }
    CheckBox {
        /* IDs need to be unique per plugin. Best way to guarantee that is to prefix with plugin name. */
        id: testPluginLogRenderEnd
        text: "Log Render End"
    }
    CheckBox {
        /* IDs need to be unique per plugin. Best way to guarantee that is to prefix with plugin name. */
        id: testPluginLogFrameSwap
        text: "Log Frame Swap"
    }
    CheckBox {
        /* IDs need to be unique per plugin. Best way to guarantee that is to prefix with plugin name. */
        id: testPluginLockMouse
        text: "Lock Mouse"
    }

    Row {
        Label {
            text: "Render Size Factor"
            anchors.verticalCenter: parent.verticalCenter
        }

        Slider {
            id: testPluginRenderSizeFac
            from: 0.5
            to: 1
            value: 1
        }
    }
}
