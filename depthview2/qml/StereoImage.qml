import QtQuick 2.5
import DepthView 2.0
import QtQuick.Controls 2.0

Item {
    width: (imageMode == SourceMode.SidebySide || imageMode == SourceMode.SidebySideAnamorphic) ? img.width / 2 : img.width
    height: (imageMode == SourceMode.TopBottom || imageMode == SourceMode.TopBottomAnamorphic) ? img.height / 2 : img.height

    scale: 1.0

    /* Wrap properties for the image. */
    property alias source: img.source
    property alias asynchronous: img.asynchronous
    property alias sourceSize: img.sourceSize
    property alias imageMode: shader.stereoMode

    BusyIndicator {
        anchors.centerIn: parent
        running: img.status === Image.Loading
    }
    Image {
        id: img
        asynchronous: true

        width: (imageMode == SourceMode.SidebySideAnamorphic) ? implicitWidth * 2 : implicitWidth
        height: (imageMode == SourceMode.TopBottomAnamorphic) ? implicitHeight * 2 : implicitHeight

        /* Hide the image, it's just used as a source for the ShaderEffect. */
        opacity: 0
    }
    StereoShader {
        id: shader
        target: img
        stereoMode: SourceMode.SidebySide
        swap: DepthView.swapEyes
    }
}

