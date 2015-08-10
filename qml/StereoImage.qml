import QtQuick 2.5

Rectangle {
    width: img.sourceSize.width / 2
    height: img.sourceSize.height

    clip: true;

    scale: 1.0

    property string source: "qrc:/test.pns"

    Image {
        x: DV.isLeft ? -parent.width : 0
        id: img
        source: parent.source
    }
}

