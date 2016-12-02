import QtQuick 2.5
import DepthView 2.0

Item {
    width: (imageMode == SourceMode.SidebySide || imageMode == SourceMode.SidebySideAnamorphic) ? img.width / 2 : img.width
    height: (imageMode == SourceMode.TopBottom || imageMode == SourceMode.TopBottomAnamorphic) ? img.height / 2 : img.height

    scale: 1.0

    /* Wrap properties for the image. */
    property url source: "qrc:/images/test.pns"
    property bool asynchronous: true
    property size sourceSize
    property int imageMode: SourceMode.SidebySide

    Image {
        id: img
        source: parent.source
        asynchronous: parent.asynchronous
        sourceSize: parent.sourceSize

        width: (imageMode == SourceMode.SidebySideAnamorphic) ? implicitWidth * 2 : implicitWidth
        height: (imageMode == SourceMode.TopBottomAnamorphic) ? implicitHeight * 2 : implicitHeight

        /* Hide the image, it is just used as a source for the ShaderEffect. */
        opacity: 0
    }
    StereoShader {
        id: shader
        target: img
        stereoMode: imageMode
        swap: DepthView.swapEyes
    }
}

