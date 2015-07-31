import QtQuick 2.5

Rectangle {
    color: "black"

    StereoImage {
        anchors.centerIn: parent
        id: image
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
                x: isLeft ? 4 : -4
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
            console.log("wheel event, scale:", image.scale, "angleDelta:", wheel.angleDelta)
            image.scale += wheel.angleDelta.y * image.scale * 0.001
            image.scale = Math.max(0.2, Math.min(image.scale, 4.0))

            console.log("scale:", image.scale)
        }
    }
}

