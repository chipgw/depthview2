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

uniform bool mirrorL;
uniform bool mirrorR;

void main(void) {
    vec2 uv = texCoord;
    uv.t *= 2.0;
    if (uv.t > 1.0) {
        uv.t -= 1.0;
        if(mirrorR)
            uv.t = 1.0 - uv.t;

        gl_FragColor = vec4(texture2D(textureR, uv).rgb, 1.0);
    } else if (uv.s < 1.0) {
        if(mirrorL)
            uv.t = 1.0 - uv.t;

        gl_FragColor = vec4(texture2D(textureL, uv).rgb, 1.0);
    } else {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

