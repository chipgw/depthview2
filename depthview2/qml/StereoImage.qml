import QtQuick 2.5

Item {
    width: img.width / 2
    height: img.height

    clip: true;

    scale: 1.0

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

