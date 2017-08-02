uniform sampler2D texture;
varying highp vec2 texCoord;

uniform vec4 rect;
uniform float outputFac;

void main() {
    gl_FragColor = texture2D(texture, texCoord * rect.zw + rect.xy) * outputFac;
}

