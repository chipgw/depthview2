import QtQuick 2.0
import QtQuick.Controls 2.0
/* For LabeledSlider */
import "qrc:/qml"

Column {
    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read and store/load. */
    property QtObject settings: QtObject {
        property bool lockMouse: false
        property real screenCurve: 0
        property real screenSize: 7
        property real screenDistance: 8
        property real screenHeight: 2
        property real renderSizeFac: 1
    }

    function reset() {
        openVR_LockMouse.checked = settings.lockMouse
        openVR_ScreenCurve.value = settings.screenCurve
        openVR_ScreenDistance.value = settings.screenDistance
        openVR_ScreenSize.value = settings.screenSize
        openVR_ScreenHeight.value = settings.screenHeight
        openvr_renderSizeFac.value = settings.renderSizeFac
    }
    function apply() {
        settings.lockMouse = openVR_LockMouse.checked
        settings.screenCurve = openVR_ScreenCurve.value
        settings.screenDistance = openVR_ScreenDistance.value
        settings.screenSize = openVR_ScreenSize.value
        settings.screenHeight = openVR_ScreenHeight.value
        settings.renderSizeFac = openvr_renderSizeFac.value
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
