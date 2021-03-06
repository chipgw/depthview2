import QtQuick 2.5
import DepthView 2.0
import QtQuick.Controls 2.0

Item {
    width: (imageMode === SourceMode.SideBySide || imageMode === SourceMode.SideBySideAnamorphic) ? img.width / 2 : img.width
    height: (imageMode === SourceMode.TopBottom || imageMode === SourceMode.TopBottomAnamorphic) ? img.height / 2 : img.height

    /* Wrap properties for the image. */
    property alias source: img.source
    property alias asynchronous: img.asynchronous
    property alias sourceSize: img.sourceSize
    property alias imageMode: shader.stereoMode
    property alias status: img.status
    property alias swap: shader.swap

    readonly property alias sourceImage: img

    Image {
        id: img
        asynchronous: true

        width: (imageMode === SourceMode.SideBySideAnamorphic) ? implicitWidth * 2 : implicitWidth
        height: (imageMode === SourceMode.TopBottomAnamorphic) ? implicitHeight * 2 : implicitHeight
    }
    StereoShader {
        id: shader
        target: img
    }
}
