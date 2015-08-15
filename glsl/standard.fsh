uniform sampler2D textureL;
uniform sampler2D textureR;
varying highp vec2 texCoord;

uniform bool left;

void main() {
    gl_FragColor = texture2D(left ? textureL : textureR, texCoord);
}

