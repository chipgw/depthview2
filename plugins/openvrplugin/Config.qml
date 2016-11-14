import QtQuick 2.0
import QtQuick.Controls 2.0

Column {
    /* Properties for C++ to read. */
    property alias lockMouse: openVR_LockMouse.checked
    property alias curvedScreen: openVR_CurvedScreen.checked
    property alias screenSize: openVR_ScreenSize.value
    property alias screenDistance: openVR_ScreenDistance.value
    property alias screenHeight: openVR_ScreenHeight.value

    MenuItem {
        id: openVR_LockMouse
        text: "Lock Mouse"
        checkable: true
    }
    MenuItem {
        id: openVR_CurvedScreen
        text: "Curved Screen"
        checkable: true
    }

    MenuItem {
        text: "Screen Size"

        onTriggered: openVR_ScreenSizePopup.open()

        Popup {
            id: openVR_ScreenSizePopup

            Slider {
                id: openVR_ScreenSize

                value: 7

                from: 1
                to: screenDistance * 2
            }
        }
    }

    MenuItem {
        text: "Screen Distance"

        onTriggered: openVR_ScreenDistancePopup.open()

        Popup {
            id: openVR_ScreenDistancePopup

            Slider {
                id: openVR_ScreenDistance

                value: 8

                from: 1
                to: 100
            }
        }
    }

    MenuItem {
        text: "Screen Height"

        onTriggered: openVR_ScreenHeightPopup.open()

        Popup {
            id: openVR_ScreenHeightPopup

            Slider {
                id: openVR_ScreenHeight

                value: 2

                from: 1
                to: 40
            }
        }
    }
}
