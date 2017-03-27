uniform sampler2D texture;
varying highp vec2 texCoord;

uniform vec4 leftRect;
uniform vec4 rightRect;

void main() {
    gl_FragData[0] = texture2D(texture, texCoord * leftRect.zw + leftRect.xy);
    gl_FragData[1] = texture2D(texture, texCoord * rightRect.zw + rightRect.xy);
}

