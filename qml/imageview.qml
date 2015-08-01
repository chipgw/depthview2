import QtQuick 2.5
import DepthView 2.0

Rectangle {
    color: "black"

    StereoImage {
        anchors.centerIn: parent
        id: image
    }

    MouseArea {
        width: viewMode.width + 20
        height: viewMode.height + 10

        Rectangle {
            anchors.fill: parent
            Text {
                anchors.centerIn: parent
                id: viewMode
                text: "Mode: " + DV.drawMode
            }
        }

        onClicked: {
            switch(DV.drawMode) {
            case DepthView.AnglaphFull:
                DV.drawMode = DepthView.AnglaphHalf
                break;
            case DepthView.AnglaphHalf:
                DV.drawMode = DepthView.AnglaphGrey
                break;
            case DepthView.AnglaphGrey:
                DV.drawMode = DepthView.AnglaphFull
                break;
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        propagateComposedEvents: true

        Rectangle {
            x: parent.mouseX
            y: parent.mouseY

            id: fakeCursor
            width: 10
            height: 10
            color: "white"

            /* This puts the cursor a little bit above the screen. */
            transform: Translate {
                x: DV.isLeft ? 4 : -4
            }
        }

        cursorShape: "BlankCursor"

        onEntered:
            fakeCursor.visible = true;
        onExited:
            fakeCursor.visible = false;

        property int lastMouseX;
        property int lastMouseY;

        onPositionChanged: {
            mouse.accepted = false

            if(mouse.buttons & Qt.LeftButton) {
                image.panX -= lastMouseX - mouse.x
                image.panY -= lastMouseY - mouse.y
            }

            lastMouseX = mouse.x
            lastMouseY = mouse.y
        }

        onWheel: {
            image.scale += wheel.angleDelta.y * image.scale * 0.001
            image.scale = Math.max(0.2, Math.min(image.scale, 4.0))
        }
    }
}

