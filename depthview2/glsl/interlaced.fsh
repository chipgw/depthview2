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
    bool left = (vertical ? mod(float(screenCoord.x), 2.0) : 0.0) == (horizontal ? mod(float(screenCoord.y), 2.0) : 0.0);
    gl_FragColor = vec4((left ? texture2D(textureL, texCoord) : texture2D(textureR, texCoord)).rgb, 1.0);
}

