import QtQuick 2.5
import DepthView 2.0

ShaderEffect {
    anchors.fill: parent

    /* Properties for the owner to set. */
    property variant target
    property int stereoMode

    /* Properties for the shader to read. */
    property bool swap: false
    readonly property bool isSBS: stereoMode === SourceMode.SidebySide || stereoMode === SourceMode.SidebySideAnamorphic
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
                    rightCoord.x += 0.5;
                else
                    leftCoord.x += 0.5;
            } else if (isTB) {
                leftCoord.y /= 2.0;
                rightCoord.y /= 2.0;

                if (swap)
                    rightCoord.y += 0.5;
                else
                    leftCoord.y += 0.5;
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
