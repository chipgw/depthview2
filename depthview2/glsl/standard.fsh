uniform sampler2D textureL;
uniform sampler2D textureR;
varying highp vec2 texCoord;

uniform bool left;

void main() {
    gl_FragColor = vec4((left ? texture2D(textureL, texCoord) : texture2D(textureR, texCoord)).rgb, 1.0);
}

