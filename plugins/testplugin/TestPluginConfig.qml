import QtQuick 2.0
import QtQuick.Controls 2.0
/* For LabeledSlider */
import "qrc:/qml"

Column {
    readonly property string title: "Test Plugin Settings"

    /* Properties for C++ to read and store/load. */
    property QtObject settings: QtObject {
        property bool logRenderStart: false
        property bool logRenderEnd: false
        property bool logFrameSwap: false
        property bool lockMouse: false
        property real renderSizeFactor: 1
    }

    function reset() {
        logRenderStartCheckBox.checked = settings.logRenderStart
        logRenderEndCheckBox.checked = settings.logRenderEnd
        logFrameSwapCheckBox.checked = settings.logFrameSwap
        lockMouseCheckBox.checked = settings.lockMouse
        renderSizeFacSlider.value = settings.renderSizeFactor
    }
    function apply() {
        settings.logRenderStart = logRenderStartCheckBox.checked
        settings.logRenderEnd = logRenderEndCheckBox.checked
        settings.logFrameSwap = logFrameSwapCheckBox.checked
        settings.lockMouse = lockMouseCheckBox.checked
        settings.renderSizeFactor = renderSizeFacSlider.value
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
