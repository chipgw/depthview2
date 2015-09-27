/* GLES requires the precision to be set but some desktop cards don't like it. */
#ifdef GL_ES
/* If highp is supported use it. */
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

uniform sampler2D textureL;
uniform sampler2D textureR;
varying highp vec2 texCoord;

uniform float greyFac;

vec3 getColor(sampler2D texture) {
    vec3 color = texture2D(texture, texCoord).rgb;
    float grey = dot(color, vec3(0.299, 0.587, 0.114)) * greyFac;
    color *= 1.0 - greyFac;
    color += grey;

    return color;
}

void main() {
    gl_FragColor = vec4(getColor(textureL).r, getColor(textureR).gb, 1.0);
}

