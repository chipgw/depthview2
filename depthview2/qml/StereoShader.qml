import QtQuick 2.5
import DepthView 2.0

ShaderEffect {
    anchors.fill: parent

    /* Properties for the owner to set. */
    property variant target
    /* Default to the current file's stereo mode. */
    property int stereoMode: FolderListing.currentFileStereoMode

    onTargetChanged: if (target !== undefined) target.visible = false

    /* Properties for the shader to read. */
    /* Default to the current file's swap value. */
    property bool swap: FolderListing.currentFileStereoSwap
    readonly property bool isSBS: stereoMode === SourceMode.SideBySide || stereoMode === SourceMode.SideBySideAnamorphic
    readonly property bool isTB: stereoMode === SourceMode.TopBottom || stereoMode === SourceMode.TopBottomAnamorphic

    vertexShader: "
        uniform highp mat4 qt_Matrix;
        attribute highp vec4 qt_Vertex;
        attribute highp vec2 qt_MultiTexCoord0;
        varying highp vec2 leftCoord;
        varying highp vec2 rightCoord;
        uniform bool isSBS;
        uniform bool isTB;
        uniform bool swap;

        void main() {
            leftCoord = qt_MultiTexCoord0;
            rightCoord = qt_MultiTexCoord0;
            if (isSBS) {
                leftCoord.x /= 2.0;
                rightCoord.x /= 2.0;

                if (swap)
                    leftCoord.x += 0.5;
                else
                    rightCoord.x += 0.5;
            } else if (isTB) {
                leftCoord.y /= 2.0;
                rightCoord.y /= 2.0;

                if (swap)
                    leftCoord.y += 0.5;
                else
                    rightCoord.y += 0.5;
            }
            gl_Position = qt_Matrix * qt_Vertex;
        }"
    fragmentShader: "
        varying highp vec2 leftCoord;
        varying highp vec2 rightCoord;
        uniform sampler2D target;
        uniform lowp float qt_Opacity;

        void main() {
            gl_FragData[0] = texture2D(target, leftCoord) * qt_Opacity;
            gl_FragData[1] = texture2D(target, rightCoord) * qt_Opacity;
        }"
}
