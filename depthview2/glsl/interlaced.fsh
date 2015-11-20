#version 130 // This shader needs bitwise integer operators, which will probably be a problem on GLES 2...

uniform sampler2D textureL;
uniform sampler2D textureR;

varying highp vec2 texCoord;

uniform bool horizontal;
uniform bool vertical;

uniform highp vec2 windowCorner;
uniform highp vec2 windowSize;

void main() {
    ivec2 screenCoord = ivec2(texCoord * windowSize);
    screenCoord.y = int(windowSize.y) - screenCoord.y;
    screenCoord += ivec2(windowCorner);
    bool left = (vertical ? (screenCoord.x & 1) : 0) == (horizontal ? (screenCoord.y & 1) : 0);
    gl_FragColor = left ? texture2D(textureL, texCoord) : texture2D(textureR, texCoord);
}

