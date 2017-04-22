/* GLES requires the precision to be set but some desktop cards don't like it. */
#ifdef GL_ES
/* If highp is supported use it. */
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

uniform sampler2D texture;
varying highp vec2 texCoord;

uniform vec4 leftRect;
uniform vec4 rightRect;

void main() {
    gl_FragData[0] = texture2D(texture, texCoord * leftRect.zw + leftRect.xy);
    gl_FragData[1] = texture2D(texture, texCoord * rightRect.zw + rightRect.xy);
}

