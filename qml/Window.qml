import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import DepthView 2.0

Rectangle {
    color: "black"

    Flickable {
        id: imageFlickable
        anchors.fill: parent

        contentWidth: imageContainer.width
        contentHeight: imageContainer.height

        /* Only enable panning when the image is zoomed in enough. */
        interactive: (contentWidth > width || contentHeight > height)

        /* The location that the center of the screen is focused on. */
        property point oldCenter: Qt.point(0.5, 0.5)

        /* Used to track differences in the visible screen area. */
        property size oldSize
        property size oldContentSize

        visibleArea.onHeightRatioChanged: {
            /* Only run if  height or contentHeight has changed and niether is zero. */
            if ((oldSize.height != height || oldContentSize.height != contentHeight) && height != 0 && contentHeight != 0) {
                /* Find out the difference between how many pixels were hidden and how many are hidden now. */
                var hidden = (oldContentSize.height - oldSize.height) - (contentHeight - height)

                /* Multiply by the calculated center to get the offset needed to maintain center. */
                contentY -= hidden * oldCenter.y

                /* Make sure we didn't get out of bounds. Needed when zooming out. */
                returnToBounds()

                /* Store the values for next time. */
                oldCenter.y = visibleArea.yPosition + visibleArea.heightRatio / 2
                oldContentSize.height = contentHeight
                oldSize.height = height
            }
        }
        visibleArea.onWidthRatioChanged: {
            /* Only run if either width or contentWidth has changed and niether is zero. */
            if ((oldSize.width != width || oldContentSize.width != contentWidth) && width != 0 && contentWidth != 0) {
                /* Find out the difference between how many pixels were hidden and how many are hidden now. */
                var hidden = (oldContentSize.width - oldSize.width) - (contentWidth - width)

                /* Multiply by the calculated center to get the offset needed to maintain center. */
                contentX -= hidden * oldCenter.x

                /* Make sure we didn't get out of bounds. Needed when zooming out. */
                returnToBounds()

                /* Store the values for next time. */
                oldCenter.x = visibleArea.xPosition + visibleArea.widthRatio / 2
                oldContentSize.width = contentWidth
                oldSize.width = width
            }
        }

        Item {
            id: imageContainer
            width: Math.max(image.width * image.scale, imageFlickable.width)
            height: Math.max(image.height * image.scale, imageFlickable.height)

            StereoImage {
                anchors.centerIn: parent
                id: image
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.MiddleButton

        onWheel: {
            /* Don't zoom if covered. */
            if (!fileBrowser.visible) {
                image.scale += wheel.angleDelta.y * image.scale * 0.001
                image.scale = Math.max(0.2, Math.min(image.scale, 4.0))
            }
        }
    }

    Item {
        anchors {
            fill: parent
            leftMargin: 8
            rightMargin: 8
        }

        /* Put all interface items a bit above the screen. */
        transform: Translate {
            x: DV.isLeft ? 8 : -8
        }

        Rectangle {
            id: menuRect
            width: parent.width

            RowLayout {
                Button {
                    text: "Pick Mode"

                    onClicked: modeDialog.visible = !modeDialog.visible
                }

                Button {
                    text: "Open"

                    onClicked: fileBrowser.visible = true
                }

                Label {
                    text: "Grey Factor:"

                    visible: DV.drawMode == DepthView.Anglaph
                }

                Slider {
                    value: DV.greyFac
                    visible: DV.drawMode == DepthView.Anglaph

                    onValueChanged: DV.greyFac = value
                }

                CheckBox {
                    visible: DV.drawMode == DepthView.SidebySide || DV.drawMode == DepthView.TopBottom
                    text: "Anamorphic"

                    checked: DV.anamorphicDualView

                    onCheckedChanged: DV.anamorphicDualView = checked
                }

                CheckBox {
                    visible: DV.drawMode == DepthView.SidebySide || DV.drawMode == DepthView.TopBottom
                    text: "Mirror Left"

                    checked: DV.mirrorLeft

                    onCheckedChanged: DV.mirrorLeft = checked
                }

                CheckBox {
                    visible: DV.drawMode == DepthView.SidebySide || DV.drawMode == DepthView.TopBottom
                    text: "Mirror Right"

                    checked: DV.mirrorRight

                    onCheckedChanged: DV.mirrorRight = checked
                }
            }
        }

        GroupBox {
            id: modeDialog
            title: "Draw Mode"
            anchors.centerIn: parent

            /* Hide by default and don't enable when hidden. */
            visible: false
            enabled: visible

            ExclusiveGroup { id: drawModeRadioGroup }

            ColumnLayout {
                RadioButton {
                    text: "Anglaph"
                    exclusiveGroup: drawModeRadioGroup

                    /* TODO - Don't make assumptions about the default value. */
                    checked: true

                    onCheckedChanged: {
                        if (checked)
                            DV.drawMode = DepthView.Anglaph
                    }
                }
                RadioButton {
                    text: "Side-by-Side"
                    exclusiveGroup: drawModeRadioGroup

                    onCheckedChanged: {
                        if (checked)
                            DV.drawMode = DepthView.SidebySide
                    }
                }
                RadioButton {
                    text: "Top/Bottom"
                    exclusiveGroup: drawModeRadioGroup

                    onCheckedChanged: {
                        if (checked)
                            DV.drawMode = DepthView.TopBottom
                    }
                }
                RadioButton {
                    text: "Mono Left"
                    exclusiveGroup: drawModeRadioGroup

                    onCheckedChanged: {
                        if (checked)
                            DV.drawMode = DepthView.MonoLeft
                    }
                }
                RadioButton {
                    text: "Mono Right"
                    exclusiveGroup: drawModeRadioGroup

                    onCheckedChanged: {
                        if (checked)
                            DV.drawMode = DepthView.MonoRight
                    }
                }
                Button {
                    text: "Close"

                    onClicked: modeDialog.visible = false
                }
            }
        }
    }

    FileBrowser {
        id: fileBrowser
        anchors.fill: parent
        visible: false
        enabled: visible

        onFileOpened: {
            visible = false
            image.source = fileURL
        }
    }

    Image {
        id: fakeCursor
        source: "qrc:/images/cursor.png"

        Connections {
            target: DV

            onMouseMoved: {
                fakeCursor.x = pos.x
                fakeCursor.y = pos.y
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DV.isLeft ? 10 : -10
        }
    }
}

