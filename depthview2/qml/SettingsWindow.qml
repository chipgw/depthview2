import QtQuick 2.5
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3
import DepthView 2.0

Dialog {
    id: root
    modal: true

    function reset() {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            if (swipe.contentModel.get(i).reset !== undefined)
                swipe.contentModel.get(i).reset();
    }

    onRejected: reset()

    onAccepted: {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            if (swipe.contentModel.get(i).apply !== undefined)
                swipe.contentModel.get(i).apply();
    }

    footer: DialogButtonBox {
        standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel | DialogButtonBox.Apply | DialogButtonBox.Reset

        onClicked: {
            if (button === standardButton(DialogButtonBox.Apply))
                swipe.currentItem.apply()
            else if (button === standardButton(DialogButtonBox.Reset))
                swipe.currentItem.reset()
        }

        Component.onCompleted: {
            /* Disable the buttons if there is no function for them to call. */
            standardButton(DialogButtonBox.Apply).enabled = Qt.binding(function() { return swipe.currentItem !== null && swipe.currentItem.apply !== undefined })
            standardButton(DialogButtonBox.Reset).enabled = Qt.binding(function() { return swipe.currentItem !== null && swipe.currentItem.reset !== undefined })
        }
    }

    /* Use the reset function to init values. */
    Component.onCompleted: reset()

    clip: true

    header: TabBar {
        id: bar
        currentIndex: swipe.currentIndex

        Repeater {
            model: swipe.contentChildren

            TabButton { text: modelData.title }
        }
    }

    Item {
        anchors.fill: parent
        clip: true

        SwipeView {
            id: swipe
            anchors.fill: parent
            currentIndex: bar.currentIndex

            Flickable {
                function reset() {
                    saveWindowStateCheckBox.checked = DepthView.saveWindowState
                    startupFileBrowserCheckBox.checked = DepthView.startupFileBrowser
                    uiThemeComboBox.currentIndex = uiThemeComboBox.find(DepthView.uiTheme)
                    startDir.text = FolderListing.startDir
                    hardwareAcceleratedVideoCheckBox.checked = DepthView.hardwareAcceleratedVideo

                    /* If the selection is invalid, use "Default", as that's what QML uses when none is set. */
                    if (uiThemeComboBox.currentIndex < 0)
                        uiThemeComboBox.currentIndex = 0
                }

                function apply() {
                    DepthView.saveWindowState = saveWindowStateCheckBox.checked
                    DepthView.startupFileBrowser = startupFileBrowserCheckBox.checked
                    DepthView.uiTheme = uiThemeComboBox.currentText
                    FolderListing.startDir = startDir.text
                    DepthView.hardwareAcceleratedVideo = hardwareAcceleratedVideoCheckBox.checked
                }

                readonly property string title: qsTr("General Settings")

                ScrollBar.vertical: ScrollBar { }
                contentHeight: generalSettingsContent.childrenRect.height

                /* Only enable panning when the item is tall enough. */
                interactive: contentHeight > height

                ColumnLayout {
                    id: generalSettingsContent
                    width: parent.width

                    CheckBox {
                        id: saveWindowStateCheckBox
                        text: qsTr("Remember Window State")
                    }

                    CheckBox {
                        id: startupFileBrowserCheckBox
                        text: qsTr("Show File Browser at Startup")
                    }

                    CheckBox {
                        id: hardwareAcceleratedVideoCheckBox
                        text: qsTr("Use Hardware Accelerated Video Decoding When Available")
                    }

                    RowLayout {
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("UI Theme: ")
                        }

                        ComboBox {
                            id: uiThemeComboBox

                            model: DepthView.uiThemes

                            Layout.fillWidth: true
                        }
                    }

                    Button {
                        /* If the function is undefined this build doesn't support file association. */
                        visible: DepthView.registerFileTypes !== undefined

                        /* TODO - It would be nice to have a more configurable "register these types" setup... */
                        text: qsTr("Register stereo image files with DepthView.")

                        onClicked: DepthView.registerFileTypes()
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Startup Directory")

                        RowLayout {
                            width: parent.width
                            TextField {
                                id: startDir
                                Layout.fillWidth: true

                                placeholderText: qsTr("Startup Directory Path...")
                                validator: FileValidator {
                                    filterDir: true
                                }
                                color: acceptableInput ? "green" : "red"
                            }

                            Button {
                                text: qsTr("Use Current")

                                onClicked: startDir.text = FolderListing.decodeURL(FolderListing.currentDir)
                            }
                        }
                    }

                    RowLayout {
                        Button {
                            text: qsTr("Reset File Database")

                            onClicked: resetFileDB.open()
                        }
                        Button {
                            text: qsTr("Reset Plugin Database")

                            onClicked: resetPluginDB.open()
                        }
                    }
                }
                Dialog {
                    id: resetFileDB
                    title: "Are you sure?"
                    modal: true

                    Label {
                        text: "All stored file information will be lost..."
                    }

                    standardButtons: Dialog.Ok | Dialog.Cancel

                    onAccepted: FolderListing.resetFileDatabase()
                }
                Dialog {
                    id: resetPluginDB
                    title: "Are you sure?"
                    modal: true

                    Label {
                        text: "All currently enabled plugins will be reset..."
                    }

                    standardButtons: Dialog.Ok | Dialog.Cancel

                    onAccepted: PluginManager.resetPluginDatabase()
                }
            }

            Flickable {
                function reset() {
                    greyFacLSlider.value = DepthView.greyFacL
                    greyFacRSlider.value = DepthView.greyFacR
                    mirrorLeftCheckBox.checked = DepthView.mirrorLeft
                    mirrorRightCheckBox.checked = DepthView.mirrorRight
                    anamorphicCheckBox.checked = DepthView.anamorphicDualView
                    swapEyesCheckBox.checked = DepthView.swapEyes
                }

                function apply() {
                    DepthView.greyFacL = greyFacLSlider.value
                    DepthView.greyFacR = greyFacRSlider.value
                    DepthView.mirrorLeft = mirrorLeftCheckBox.checked
                    DepthView.mirrorRight = mirrorRightCheckBox.checked
                    DepthView.anamorphicDualView = anamorphicCheckBox.checked
                    DepthView.swapEyes = swapEyesCheckBox.checked
                }

                readonly property string title: qsTr("Render Settings")

                ScrollBar.vertical: ScrollBar { }
                contentHeight: renderSettingsContent.childrenRect.height

                /* Only enable panning when the item is tall enough. */
                interactive: contentHeight > height

                ColumnLayout {
                    id: renderSettingsContent
                    width: parent.width

                    GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Anaglyph")

                        ColumnLayout {
                            anchors.fill: parent
                            LabeledSlider {
                                text: qsTr("Grey Factor (Left Eye)")

                                id: greyFacLSlider
                                Layout.fillWidth: true
                            }

                            LabeledSlider {
                                text: qsTr("Grey Factor (Right Eye)")

                                id: greyFacRSlider
                                Layout.fillWidth: true
                            }
                        }
                    }

                    GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Side-by-Side/Top-Bottom")

                        Column {
                            anchors.fill: parent

                            CheckBox {
                                id: mirrorLeftCheckBox
                                text: qsTr("Mirror Left")
                            }

                            CheckBox {
                                id: mirrorRightCheckBox
                                text: qsTr("Mirror Right")
                            }

                            CheckBox {
                                id: anamorphicCheckBox
                                text: qsTr("Anamorphic")
                            }
                        }
                    }

                    CheckBox {
                        id: swapEyesCheckBox
                        text: qsTr("Swap Eyes")
                    }
                }
            }

            Flickable {
                function reset() {
                    lockMouse.checked = VRManager.lockMouse
                    mirrorUI.checked = VRManager.mirrorUI
                    snapSurroundPan.checked = VRManager.snapSurroundPan
                    screenCurve.value = VRManager.screenCurve
                    screenDistance.value = VRManager.screenDistance
                    screenSize.value = VRManager.screenSize
                    screenHeight.value = VRManager.screenHeight
                    renderSizeFac.value = VRManager.renderSizeFac
                    backgroundImagePath.text = FolderListing.decodeURL(VRManager.backgroundImage)
                    backgroundImageMode.setMode(VRManager.backgroundSourceMode)
                    backgroundImageSwap.checked = VRManager.backgroundSwap
                    backgroundImagePan.value = VRManager.backgroundPan
                    backgroundImageDim.value = VRManager.backgroundDim
                }
                function apply() {
                    VRManager.lockMouse = lockMouse.checked
                    VRManager.mirrorUI = mirrorUI.checked
                    VRManager.snapSurroundPan = snapSurroundPan.checked
                    VRManager.screenCurve = screenCurve.value
                    VRManager.screenDistance = screenDistance.value
                    VRManager.screenSize = screenSize.value
                    VRManager.screenHeight = screenHeight.value
                    VRManager.renderSizeFac = renderSizeFac.value
                    /* TODO - Check if it exists. */
                    VRManager.backgroundImage = FolderListing.encodeURL(backgroundImagePath.text)
                    VRManager.backgroundSourceMode = backgroundImageMode.mode
                    VRManager.backgroundSwap = backgroundImageSwap.checked
                    VRManager.backgroundPan = backgroundImagePan.value
                    VRManager.backgroundDim = backgroundImageDim.value
                }

                readonly property string title: qsTr("Virtual Reality Settings")

                ScrollBar.vertical: ScrollBar { }
                contentHeight: vrSettingsContent.childrenRect.height

                /* Only enable panning when the item is tall enough. */
                interactive: contentHeight > height

                ColumnLayout {
                    id: vrSettingsContent
                    width: parent.width

                    Flow {
                        Layout.fillWidth: true
                        CheckBox {
                            id: lockMouse
                            text: qsTr("Lock Mouse")
                        }
                        CheckBox {
                            id: mirrorUI
                            text: qsTr("Mirror UI to Window")
                        }
                        CheckBox {
                            id: snapSurroundPan
                            text: qsTr("Snap Surround Image Pan")
                        }
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

                    GroupBox {
                        Layout.fillWidth: true
                        title: qsTr("Background")
                        GridLayout {
                            /* TODO - Still not quite happy with this layout... */
                            width: parent.width
                            columns: 7

                            TextField {
                                id: backgroundImagePath
                                Layout.columnSpan: 6
                                Layout.fillWidth: true

                                placeholderText: qsTr("Background Image Path...")
                                validator: FileValidator {
                                    filterSurround: true
                                    folderListing: FolderListing
                                }
                                color: acceptableInput ? "green" : "red"
                            }
                            Button {
                                text: "Use Current"
                                Layout.columnSpan: 1

                                enabled: FolderListing.currentFileIsSurround

                                onClicked: {
                                    backgroundImagePath.text = FolderListing.decodeURL(FolderListing.currentDir) + "/" + FolderListing.currentFile
                                    backgroundImageMode.setMode(FolderListing.currentFileStereoMode)
                                    backgroundImageSwap.checked = FolderListing.currentFileStereoSwap
                                    backgroundImagePan.value = DepthView.surroundPan.x
                                }
                            }

                            CheckBox {
                                id: backgroundImageSwap
                                text: "Swap"
                                Layout.fillWidth: true

                                enabled: backgroundImageMode.mode !== SourceMode.Mono
                            }

                            Label {
                                text: qsTr("Source Mode:")
                            }
                            ComboBox {
                                id: backgroundImageMode
                                textRole: "text"
                                model: ListModel {
                                    ListElement { text: qsTr("Mono"); mode: SourceMode.Mono }
                                    ListElement { text: qsTr("Side-by-Side"); mode: SourceMode.SideBySide }
                                    ListElement { text: qsTr("Top/Bottom"); mode: SourceMode.TopBottom }
                                }
                                function setMode(mode) {
                                    if (mode === SourceMode.SideBySideAnamorphic)
                                        mode = SourceMode.SideBySide
                                    if (mode === SourceMode.TopBottomAnamorphic)
                                        mode = SourceMode.TopBottom

                                    for (var i = 0; i < model.count; ++i)
                                        if (model.get(i).mode === mode)
                                            currentIndex = i
                                }
                                readonly property var mode: model.get(currentIndex).mode
                            }

                            Label {
                                text: qsTr("Pan")
                            }

                            Slider {
                                id: backgroundImagePan
                                Layout.fillWidth: true
                                from: 0
                                to: 360
                            }

                            Label {
                                text: qsTr("Dim")
                            }

                            Slider {
                                id: backgroundImageDim
                                Layout.fillWidth: true
                                from: 0
                                to: 1
                            }
                        }
                    }
                }

                Connections {
                    target: VRManager
                    onInitedChanged: if (VRManager.isInited) reset()
                }
            }

            Flickable {
                readonly property string title: qsTr("Plugin Management")

                ScrollBar.vertical: ScrollBar { }
                contentHeight: pluginsContent.childrenRect.height

                /* Only enable panning when the item is tall enough. */
                interactive: contentHeight > height

                ColumnLayout {
                    id: pluginsContent
                    width: parent.width

                    Repeater {
                        model: PluginManager

                        GroupBox {
                            font: uiTextFont
                            title: pluginDisplayName
                            Layout.preferredWidth: pluginsContent.width

                            RowLayout {
                                width: parent.width

                                Text {
                                    font: uiTextFont
                                    Layout.fillWidth: true
                                    text: ("<b>" + pluginType + ",</b> " + pluginFileName + "<br><br>" + pluginDescription) +
                                          ((pluginError.length > 0) ? ("<br><br><font color=\"red\">" + pluginError + "</font>") : "")

                                    wrapMode: Text.WordWrap
                                }
                                CheckBox {
                                    text: "Enabled"
                                    checked: pluginEnabled
                                    onClicked:
                                        if (checked) PluginManager.enablePlugin(pluginFileName)
                                        else PluginManager.disablePlugin(pluginFileName)
                                    enabled: pluginError.length < 1
                                }
                            }
                        }
                    }
                }
            }

            Repeater {
                model: PluginManager.pluginConfigMenus

                Flickable {
                    /* Use the title property from the provided item. */
                    readonly property string title: modelData.title

                    function reset() {
                        try {
                            /* If the plugin has a settings object, load it. */
                            if (modelData.settings)
                                PluginManager.loadPluginSettings(title, modelData.settings)

                            modelData.reset()
                        } catch (e) {
                            console.warn(title, "did not reset correctly!", e)
                        }
                    }

                    function apply() {
                        try {
                            modelData.apply()

                            /* If the plugin has a settings object, save it. */
                            if (modelData.settings)
                                PluginManager.savePluginSettings(title, modelData.settings)
                        } catch (e) {
                            console.warn(title, "did not apply correctly!", e)
                        }
                    }

                    ScrollBar.vertical: ScrollBar { }
                    contentHeight: modelData.height

                    /* Only enable panning when the item is tall enough. */
                    interactive: contentHeight > height

                    Component.onCompleted: {
                        /* Attach the provided component to the Flickable's contentItem. */
                        modelData.parent = contentItem
                        /* Make the component inherit the contentItem's width. */
                        modelData.width = Qt.binding(function() { return contentItem.width })

                        /* In case they were not part of the global reset, such as when a plugin is first enabled. */
                        reset()
                    }
                }
            }
        }
    }
}
