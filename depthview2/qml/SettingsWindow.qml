import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Popup {
    id: root

    function resetAll() {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            swipe.contentModel.get(i).reset();
    }
    function applyAll() {
        for (var i = 0; i < swipe.contentModel.count; ++i)
            swipe.contentModel.get(i).apply();
    }
    function resetCurrent() {
        swipe.currentItem.reset();
    }
    function applyCurrent() {
        swipe.currentItem.apply();
    }

    function cancel() {
        resetAll()

        close()
    }
    function accept() {
        applyAll()

        close()
    }

    /* Use the reset function to init values. */
    Component.onCompleted: resetAll()

    Page {
        anchors.fill: parent

        header: TabBar {
            id: bar
            currentIndex: swipe.currentIndex

            Repeater {
                model: swipe.contentChildren

                TabButton {
                    text: modelData.title
                }
            }
        }

        footer: ToolBar {
            RowLayout {
                anchors.fill: parent

                ToolButton {
                    text: qsTr("Reset")

                    onClicked: resetCurrent()
                }
                Item {
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: qsTr("Apply")

                    onClicked: applyCurrent()
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

            clip: true

            SwipeView {
                id: swipe
                anchors.fill: parent
                currentIndex: bar.currentIndex

                Flickable {
                    function reset() {
                        saveWindowStateCheckBox.checked = DepthView.saveWindowState
                        startupFileBrowserCheckBox.checked = DepthView.startupFileBrowser
                        greyFacSlider.value = DepthView.greyFac
                        mirrorLeftCheckBox.checked = DepthView.mirrorLeft
                        mirrorRightCheckBox.checked = DepthView.mirrorRight
                        anamorphicCheckBox.checked = DepthView.anamorphicDualView
                        swapEyesCheckBox.checked = DepthView.swapEyes
                        uiThemeComboBox.currentIndex = uiThemeComboBox.find(DepthView.uiTheme)
                    }

                    function apply() {
                        DepthView.saveWindowState = saveWindowStateCheckBox.checked
                        DepthView.startupFileBrowser = startupFileBrowserCheckBox.checked
                        DepthView.greyFac = greyFacSlider.value
                        DepthView.mirrorLeft = mirrorLeftCheckBox.checked
                        DepthView.mirrorRight = mirrorRightCheckBox.checked
                        DepthView.anamorphicDualView = anamorphicCheckBox.checked
                        DepthView.swapEyes = swapEyesCheckBox.checked
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
                                text: "UI Theme: "
                            }

                            ComboBox {
                                id: uiThemeComboBox

                                /* TODO - Material mode messes up the Google Material icons. (Funny how that works...) */
                                model: ["Default", "Material", "Universal"]
                            }
                        }

                        GroupBox {
                            width: parent.width
                            title: qsTr("Anaglyph")

                            RowLayout {
                                anchors.fill: parent

                                Label {
                                    text: qsTr("Grey Factor")
                                    font: uiTextFont
                                }

                                Slider {
                                    id: greyFacSlider
                                    Layout.fillWidth: true
                                }
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
                                    font: uiTextFont
                                }

                                CheckBox {
                                    id: mirrorRightCheckBox
                                    text: qsTr("Mirror Right")
                                    font: uiTextFont
                                }

                                CheckBox {
                                    id: anamorphicCheckBox
                                    text: qsTr("Anamorphic")
                                    font: uiTextFont
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
                                modelData.reset()
                            } catch (e) {
                                console.warn(title, "did not reset correctly!", e)
                            }
                        }

                        function apply() {
                            try {
                                modelData.apply()
                            } catch (e) {
                                console.warn(title, "did not apply correctly!", e)
                            }
                        }

                        ScrollBar.vertical: ScrollBar { }
                        contentHeight: modelData.height

                        /* Only enable panning when the item is tall enough. */
                        interactive: contentHeight > height

                        Component.onCompleted: modelData.parent = contentItem
                    }
                }
            }
        }
    }
}
