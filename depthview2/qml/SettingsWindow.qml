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

                    /* If the selection is invalid, use "Default", as that's what QML uses when none is set. */
                    if (uiThemeComboBox.currentIndex < 0)
                        uiThemeComboBox.currentIndex = 0
                }

                function apply() {
                    DepthView.saveWindowState = saveWindowStateCheckBox.checked
                    DepthView.startupFileBrowser = startupFileBrowserCheckBox.checked
                    DepthView.uiTheme = uiThemeComboBox.currentText
                }

                readonly property string title: qsTr("General Settings")

                ScrollBar.vertical: ScrollBar { }
                contentHeight: generalSettingsContent.childrenRect.height

                /* Only enable panning when the item is tall enough. */
                interactive: contentHeight > height

                Column {
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

                    Row {
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("UI Theme: ")
                        }

                        ComboBox {
                            id: uiThemeComboBox

                            model: DepthView.uiThemes
                        }
                    }
                    Button {
                        /* If the function is undefined this build doesn't support file association. */
                        visible: DepthView.registerFileTypes !== undefined

                        /* TODO - It would be nice to have a more configurable "register these types" setup... */
                        text: qsTr("Register stereo image files with DepthView.")

                        onClicked: DepthView.registerFileTypes()
                    }
                    Button {
                        text: qsTr("Reset File Database")

                        /* TODO - Confirm. */
                        onClicked: FolderListing.resetFileDatabase()
                    }
                    Button {
                        text: qsTr("Reset Plugin Database")

                        /* TODO - Confirm. */
                        onClicked: PluginManager.resetPluginDatabase()
                    }
                }
            }

            Flickable {
                function reset() {
                    greyFacSlider.value = DepthView.greyFac
                    mirrorLeftCheckBox.checked = DepthView.mirrorLeft
                    mirrorRightCheckBox.checked = DepthView.mirrorRight
                    anamorphicCheckBox.checked = DepthView.anamorphicDualView
                    swapEyesCheckBox.checked = DepthView.swapEyes
                }

                function apply() {
                    DepthView.greyFac = greyFacSlider.value
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

                Column {
                    id: renderSettingsContent
                    width: parent.width

                    GroupBox {
                        width: parent.width
                        title: qsTr("Anaglyph")

                        LabeledSlider {
                            text: qsTr("Grey Factor")

                            id: greyFacSlider
                            Layout.fillWidth: true
                        }
                    }

                    GroupBox {
                        width: parent.width
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
