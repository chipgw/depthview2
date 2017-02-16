import QtQuick 2.0
import QtQuick.Controls 2.0
/* For LabeledSlider */
import "qrc:/qml"

Column {
    readonly property string title: "Test Plugin Settings"

    /* Properties for C++ to read. */
    property bool logRenderStart: false
    property bool logRenderEnd: false
    property bool logFrameSwap: false
    property bool lockMouse: false
    property real renderSizeFactor: 1.0

    function reset() {
        logRenderStartCheckBox.checked = logRenderStart
        logRenderEndCheckBox.checked = logRenderEnd
        logFrameSwapCheckBox.checked = logFrameSwap
        lockMouseCheckBox.checked = lockMouse
        renderSizeFacSlider.value = renderSizeFactor
    }
    function apply() {
        logRenderStart = logRenderStartCheckBox.checked
        logRenderEnd = logRenderEndCheckBox.checked
        logFrameSwap = logFrameSwapCheckBox.checked
        lockMouse = lockMouseCheckBox.checked
        renderSizeFactor = renderSizeFacSlider.value
    }

    CheckBox {
        id: logRenderStartCheckBox
        text: "Log Render Start"
    }
    CheckBox {
        id: logRenderEndCheckBox
        text: "Log Render End"
    }
    CheckBox {
        id: logFrameSwapCheckBox
        text: "Log Frame Swap"
    }
    CheckBox {
        id: lockMouseCheckBox
        text: "Lock Mouse"
    }

    LabeledSlider {
        id: renderSizeFacSlider

        text: "Render Size Factor"

        from: 0.5
        to: 1
    }
}
