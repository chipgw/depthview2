import QtQuick 2.5
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

Popup {
    id: root

    function reset() {
        saveWindowStateCheckBox.checked = DepthView.saveWindowState
        startupFileBrowserCheckBox.checked = DepthView.startupFileBrowser
        greyFacSlider.value = DepthView.greyFac
        mirrorLeftCheckBox.checked = DepthView.mirrorLeft
        mirrorRightCheckBox.checked = DepthView.mirrorRight
        anamorphicCheckBox.checked = DepthView.anamorphicDualView
        swapEyesCheckBox.checked = DepthView.swapEyes

        /* TODO - Reset plugin settings. */
    }

    function apply() {
        DepthView.saveWindowState = saveWindowStateCheckBox.checked
        DepthView.startupFileBrowser = startupFileBrowserCheckBox.checked
        DepthView.greyFac = greyFacSlider.value
        DepthView.mirrorLeft = mirrorLeftCheckBox.checked
        DepthView.mirrorRight = mirrorRightCheckBox.checked
        DepthView.anamorphicDualView = anamorphicCheckBox.checked
        DepthView.swapEyes = swapEyesCheckBox.checked
    }

    Page {
        anchors.fill: parent

        header: ToolBar {
            Row {
                anchors.fill: parent
                Repeater {
                    model: swipe.contentChildren

                    ToolButton {
                        text: modelData.title

                        onClicked: swipe.currentIndex = index

                        checked: swipe.currentIndex === index
                    }
                }
            }
        }

        footer: ToolBar {
            RowLayout {
                anchors.fill: parent

                ToolButton {
                    text: qsTr("Reset")

                    onClicked: reset()
                }
                Item {
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: qsTr("Apply")

                    onClicked: apply()
                }
                ToolButton {
                    text: qsTr("Ok")

                    onClicked: {
                        apply()

                        root.close()
                    }
                }

                ToolButton {
                    text: qsTr("Cancel")

                    onClicked: {
                        reset()

                        root.close()
                    }
                }
            }
        }

        Item {
            anchors.fill: parent

            clip: true

            SwipeView {
                id: swipe
                anchors.fill: parent

                Flickable {
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

                            checked: DepthView.saveWindowState
                        }

                        CheckBox {
                            id: startupFileBrowserCheckBox
                            text: qsTr("Show File Browser at Startup")

                            checked: DepthView.startupFileBrowser
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
                                    value: DepthView.greyFac
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

                                    checked: DepthView.mirrorLeft
                                }

                                CheckBox {
                                    id: mirrorRightCheckBox
                                    text: qsTr("Mirror Right")
                                    font: uiTextFont

                                    checked: DepthView.mirrorRight
                                }

                                CheckBox {
                                    id: anamorphicCheckBox
                                    text: qsTr("Anamorphic")
                                    font: uiTextFont

                                    checked: DepthView.anamorphicDualView

                                }
                            }
                        }

                        CheckBox {
                            id: swapEyesCheckBox
                            text: qsTr("Swap Eyes")

                            checked: DepthView.swapEyes
                        }
                    }
                }

                Repeater {
                    model: DepthView.pluginConfigMenus

                    Flickable {
                        /* Use the title property from the provided item. */
                        readonly property string title: modelData.title

                        ScrollBar.vertical: ScrollBar { }
                        contentHeight: modelData.childrenRect.height

                        /* Only enable panning when the item is tall enough. */
                        interactive: contentHeight > height

                        Component.onCompleted: modelData.parent = contentItem
                    }
                }
            }
        }
    }
}
