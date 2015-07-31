uniform sampler2D textureL;
uniform sampler2D textureR;
varying highp vec2 texCoordFS;

void main() {
    gl_FragColor = texture2D(textureL, texCoordFS);
}

