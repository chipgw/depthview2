import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    readonly property string title: "Test Plugin Settings"

    /* Properties for C++ to read. */
    property alias logRenderStart: testPluginLogRenderStart.checked
    property alias logRenderEnd: testPluginLogRenderEnd.checked
    property alias logFrameSwap: testPluginLogFrameSwap.checked

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
