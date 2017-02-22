import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import DepthView 2.0

Popup {
    id: root

    function reset() {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            swipe.contentModel.get(i).reset();
    }
    function cancel() {
        reset()
        close()
    }
    function accept() {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            swipe.contentModel.get(i).apply();

        close()
    }

    /* Use the reset function to init values. */
    Component.onCompleted: reset()

    Page {
        clip: true
        anchors.fill: parent

        header: TabBar {
            id: bar
            currentIndex: swipe.currentIndex

            Repeater {
                model: swipe.contentChildren

                TabButton { text: modelData.title }
            }
        }

        footer: ToolBar {
            RowLayout {
                anchors.fill: parent

                ToolButton {
                    text: qsTr("Reset")

                    onClicked: swipe.currentItem.reset()
                }
                /* Empty item for spacing. */
                Item { Layout.fillWidth: true }

                ToolButton {
                    text: qsTr("Apply")

                    onClicked: swipe.currentItem.apply()
                }
                ToolButton {
                    text: qsTr("Ok")

                    onClicked: accept()
                }

                ToolButton {
                    text: qsTr("Cancel")

                    onClicked: cancel()
                }
            }
        }

        Item {
            anchors.fill: parent

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

                                model: ["Default", "Material", "Universal"]
                            }
                        }
                        Button {
                            /* If the function is undefined this build doesn't support file association. */
                            visible: DepthView.registerFileTypes !== undefined

                            /* TODO - It would be nice to have a more configurable "register these types" setup... */
                            text: qsTr("Register stereo image files with DepthView.")

                            onClicked: DepthView.registerFileTypes()
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

                Repeater {
                    model: DepthView.pluginConfigMenus

                    Flickable {
                        /* Use the title property from the provided item. */
                        readonly property string title: modelData.title

                        function reset() {
                            try {
                                /* If the plugin has a settings object, load it. */
                                if (modelData.settings)
                                    DepthView.loadPluginSettings(title, modelData.settings)

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
                                    DepthView.savePluginSettings(title, modelData.settings)
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
                        }
                    }
                }
            }
        }
    }
}
