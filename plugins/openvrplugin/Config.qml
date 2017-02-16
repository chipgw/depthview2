import QtQuick 2.0
import QtQuick.Controls 2.0
/* For LabeledSlider */
import "qrc:/qml"

Column {
    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read. */
    property bool lockMouse: false
    property real screenCurve: 0
    property real screenSize: 7
    property real screenDistance: 8
    property real screenHeight: 2
    property real renderSizeFac: 1

    function reset() {
        openVR_LockMouse.checked = lockMouse
        openVR_ScreenCurve.value = screenCurve
        openVR_ScreenDistance.value = screenDistance
        openVR_ScreenSize.value = screenSize
        openVR_ScreenHeight.value = screenHeight
        openvr_renderSizeFac.value = renderSizeFac
    }
    function apply() {
        lockMouse = openVR_LockMouse.checked
        screenCurve = openVR_ScreenCurve.value
        screenDistance = openVR_ScreenDistance.value
        screenSize = openVR_ScreenSize.value
        screenHeight = openVR_ScreenHeight.value
        renderSizeFac = openvr_renderSizeFac.value
    }

    CheckBox {
        id: openVR_LockMouse
        text: "Lock Mouse"
    }

    LabeledSlider {
        id: openVR_ScreenCurve

        text: "Screen Curviness"

        from: 0
        to: 1
    }

    LabeledSlider {
        id: openVR_ScreenSize

        text: "Screen Size"

        from: 1
        to: openVR_ScreenDistance.value * 2
    }

    LabeledSlider {
        id: openVR_ScreenDistance

        text: "Screen Distance"

        from: 1
        to: 100
    }

    LabeledSlider {
        id: openVR_ScreenHeight

        text: "Screen Height"

        from: 1
        to: 40
    }
    LabeledSlider {
        id: openvr_renderSizeFac

        text: "Render Size Factor"

        from: 0.5
        to: 1
    }
}
