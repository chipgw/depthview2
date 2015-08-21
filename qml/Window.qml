import QtQuick 2.5
import QtQuick.Controls 1.4
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

    Rectangle {
        id: menuRect
        width: parent.width

        Flow {
            Button {
                text: "Mode: " + DV.drawMode

                onClicked: {
                    switch(DV.drawMode) {
                    case DepthView.AnglaphFull:
                        DV.drawMode = DepthView.AnglaphHalf
                        break;
                    case DepthView.AnglaphHalf:
                        DV.drawMode = DepthView.AnglaphGrey
                        break;
                    case DepthView.AnglaphGrey:
                        DV.drawMode = DepthView.SidebySide
                        break;
                    case DepthView.SidebySide:
                        DV.drawMode = DepthView.SidebySideMLeft
                        break;
                    case DepthView.SidebySideMLeft:
                        DV.drawMode = DepthView.SidebySideMRight
                        break;
                    case DepthView.SidebySideMRight:
                        DV.drawMode = DepthView.SidebySideMBoth
                        break;
                    case DepthView.SidebySideMBoth:
                        DV.drawMode = DepthView.TopBottom
                        break;
                    case DepthView.TopBottom:
                        DV.drawMode = DepthView.TopBottomMTop
                        break;
                    case DepthView.TopBottomMTop:
                        DV.drawMode = DepthView.TopBottomMBottom
                        break;
                    case DepthView.TopBottomMBottom:
                        DV.drawMode = DepthView.TopBottomMBoth
                        break;
                    case DepthView.TopBottomMBoth:
                        DV.drawMode = DepthView.MonoLeft
                        break;
                    case DepthView.MonoLeft:
                        DV.drawMode = DepthView.MonoRight
                        break;
                    case DepthView.MonoRight:
                        DV.drawMode = DepthView.AnglaphFull
                        break;
                    }
                }
            }

            Button {
                text: "Open"

                onClicked: {
                    fileBrowser.visible = true
                }
            }

            Button {
                visible: DV.isSideBySide || DV.isTopBottom
                text: "Anamorphic: " + (DV.anamorphicDualView ? "On" : "Off")

                onClicked: {
                    DV.anamorphicDualView = !DV.anamorphicDualView
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
            x: DV.isLeft ? 8 : -8
        }
    }
}

