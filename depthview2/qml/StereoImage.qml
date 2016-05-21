import QtQuick 2.5

Item {
    width: img.width / 2
    height: img.height

    /* We need to clip to hide the image for the eye not currently being shown. */
    clip: true

    scale: 1.0

    /* Wrap properties for the image. */
    property url source: "qrc:/images/test.pns"
    property bool asynchronous: false
    property size sourceSize

    Image {
        x: DepthView.isLeft ? -parent.width : 0
        id: img
        source: parent.source
        asynchronous: parent.asynchronous
        sourceSize: parent.sourceSize
    }
}

