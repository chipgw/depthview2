import QtQuick 2.5
import DepthView 2.0

Item {
    width: (imageMode == SourceMode.SidebySide || imageMode == SourceMode.SidebySideAnamorphic) ? img.width / 2 : img.width
    height: (imageMode == SourceMode.TopBottom || imageMode == SourceMode.TopBottomAnamorphic) ? img.height / 2 : img.height

    /* We need to clip to hide the image for the eye not currently being shown. */
    clip: true

    scale: 1.0

    /* Wrap properties for the image. */
    property url source: "qrc:/images/test.pns"
    property bool asynchronous: false
    property size sourceSize
    property int imageMode: SourceMode.SidebySide

    Image {
        x: (DepthView.isLeft && (imageMode == SourceMode.SidebySide || imageMode == SourceMode.SidebySideAnamorphic)) ? -width / 2 : 0
        y: (DepthView.isLeft && (imageMode == SourceMode.TopBottom || imageMode == SourceMode.TopBottomAnamorphic)) ? -height / 2 : 0

        id: img
        source: parent.source
        asynchronous: parent.asynchronous
        sourceSize: parent.sourceSize

        width: (imageMode == SourceMode.SidebySideAnamorphic) ? implicitWidth * 2 : implicitWidth
        height: (imageMode == SourceMode.TopBottomAnamorphic) ? implicitHeight * 2 : implicitHeight
    }
}

