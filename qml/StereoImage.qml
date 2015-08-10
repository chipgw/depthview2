import QtQuick 2.5

Rectangle {
    width: img.sourceSize.width / 2
    height: img.sourceSize.height

    clip: true;

    scale: 1.0

    property real panX: 0
    property real panY: 0
    property bool enablePan: true

    transform: Translate {
        x: getPanX()
        y: getPanY()
    }

    function getPanX() {
        var max = Math.max(width * scale - parent.width, 0) / 2

        return panX = Math.max(-max, Math.min(panX, max))
    }

    function getPanY() {
        var max = Math.max(height * scale - parent.height, 0) / 2

        return panY = Math.max(-max, Math.min(panY, max))
    }

    property string source: "qrc:/test.pns"

    Image {
        x: DV.isLeft ? -parent.width : 0
        id: img
        source: parent.source
    }

    MouseArea {
        hoverEnabled: true
        anchors.fill: parent

        property int lastMouseX;
        property int lastMouseY;

        onPositionChanged: {
            if(mouse.buttons & Qt.LeftButton && parent.enablePan) {
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

