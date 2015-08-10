import QtQuick 2.5
import QtQuick.Controls 1.4
import DepthView 2.0

Rectangle {
    color: "black"

    Item {
        anchors.centerIn: parent
        width: image.width
        height: image.height

        StereoImage {
            id: image
        }
    }

    MouseArea {
        anchors.fill: parent

        drag.target: image
        drag.minimumX: -Math.max(image.width * image.scale - parent.width, 0) / 2
        drag.maximumX:  Math.max(image.width * image.scale - parent.width, 0) / 2
        drag.minimumY: -Math.max(image.height * image.scale - parent.height, 0) / 2
        drag.maximumY:  Math.max(image.height * image.scale - parent.height, 0) / 2

        onWheel: {
            image.scale += wheel.angleDelta.y * image.scale * 0.001
            image.scale = Math.max(0.2, Math.min(image.scale, 4.0))
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
                        DV.drawMode = DepthView.AnglaphFull
                        break;
                    }
                }
            }

            Button {
                text: "Anamorphic: " + (DV.anamorphicDualView ? "On" : "Off")

                onClicked: {
                    DV.anamorphicDualView = !DV.anamorphicDualView
                }
            }
        }
    }

    Rectangle {
        id: fakeCursor
        width: 10
        height: 10
        color: "white"

        Connections {
            target: DV

            onMouseMoved: {
                fakeCursor.x = pos.x
                fakeCursor.y = pos.y
            }
        }

        /* This puts the cursor a little bit above the screen. */
        transform: Translate {
            x: DV.isLeft ? 4 : -4
        }
    }
}

