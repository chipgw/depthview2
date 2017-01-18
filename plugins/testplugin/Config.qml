import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Test Plugin Settings"

    /* Properties for C++ to read. */
    property bool logRenderStart: false
    property bool logRenderEnd: false
    property bool logFrameSwap: false

    function reset() {
        testPluginLogRenderStart.checked = logRenderStart
        testPluginLogRenderEnd.checked = logRenderEnd
        testPluginLogFrameSwap.checked = logFrameSwap
    }
    function apply() {
        logRenderStart = testPluginLogRenderStart.checked
        logRenderEnd = testPluginLogRenderEnd.checked
        logFrameSwap = testPluginLogFrameSwap.checked
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
}
