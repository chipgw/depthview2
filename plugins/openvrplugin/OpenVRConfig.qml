import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0
import DepthView 2.0
/* For LabeledSlider */
import "qrc:/qml"

ColumnLayout {
    readonly property string title: "OpenVR Settings"

    /* Properties for C++ to read and store/load. */
    property QtObject settings: QtObject {
        property bool lockMouse: false
        property real screenCurve: 0
        property real screenSize: 7
        property real screenDistance: 8
        property real screenHeight: 2
        property real renderSizeFac: 1
        property url backgroundImage
        property int backgroundSourceMode: SourceMode.Mono
        property bool backgroundSwap: false
    }

    function reset() {
        lockMouse.checked = settings.lockMouse
        screenCurve.value = settings.screenCurve
        screenDistance.value = settings.screenDistance
        screenSize.value = settings.screenSize
        screenHeight.value = settings.screenHeight
        renderSizeFac.value = settings.renderSizeFac
        backgroundImagePath.text = settings.backgroundImage
        backgroundImageMode.setMode(settings.backgroundSourceMode)
        backgroundImageSwap.checked = settings.backgroundSwap
    }
    function apply() {
        settings.lockMouse = lockMouse.checked
        settings.screenCurve = screenCurve.value
        settings.screenDistance = screenDistance.value
        settings.screenSize = screenSize.value
        settings.screenHeight = screenHeight.value
        settings.renderSizeFac = renderSizeFac.value
        /* TODO - Check if it exists. */
        settings.backgroundImage = backgroundImagePath.text
        settings.backgroundSourceMode = backgroundImageMode.mode
        settings.backgroundSwap = backgroundImageSwap.checked
    }

    CheckBox {
        id: lockMouse
        text: "Lock Mouse"
    }

    LabeledSlider {
        id: screenCurve

        text: "Screen Curviness"

        from: 0
        to: 1
    }

    LabeledSlider {
        id: screenSize

        text: "Screen Size"

        from: 1
        to: screenDistance.value * 2
    }

    LabeledSlider {
        id: screenDistance

        text: "Screen Distance"

        from: 1
        to: 100
    }

    LabeledSlider {
        id: screenHeight

        text: "Screen Height"

        from: 1
        to: 40
    }
    LabeledSlider {
        id: renderSizeFac

        text: "Render Size Factor"

        from: 0.5
        to: 1
    }

    RowLayout {
        width: parent.width
        TextField {
            id: backgroundImagePath
            Layout.fillWidth: true
        }

        ComboBox {
            id: backgroundImageMode
            textRole: "text"
            model: ListModel {
                ListElement { text: qsTr("Mono"); mode: SourceMode.Mono }
                ListElement { text: qsTr("Side-by-Side"); mode: SourceMode.SidebySide }
                ListElement { text: qsTr("Top/Bottom"); mode: SourceMode.TopBottom }
            }
            function setMode(mode) {
                if (mode == SourceMode.SidebySideAnamorphic)
                    mode = SourceMode.SidebySide
                if (mode == SourceMode.TopBottomAnamorphic)
                    mode = SourceMode.TopBottom

                for (var i = 0; i < model.count; ++i)
                    if (model.get(i).mode == mode)
                        currentIndex = i
            }
            readonly property var mode: model.get(currentIndex).mode
        }
        CheckBox {
            id: backgroundImageSwap
            text: "Swap"

            enabled: backgroundImageMode.mode != SourceMode.Mono
        }

        Button {
            text: "Use Current"

            enabled: FolderListing.currentFileIsSurround

            onClicked: {
                backgroundImagePath.text = FolderListing.currentDir + "/" + FolderListing.currentFile
                backgroundImageMode.setMode(FolderListing.currentFileStereoMode)
            }
        }
    }
    Image {
        objectName: "backgroundImage"
        source: settings.backgroundImage
        visible: false
    }
}
