precision highp float;

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

        gl_FragColor = texture2D(textureR, uv);
    } else if (uv.s < 1.0) {
        if(mirrorL)
            uv.t = 1.0 - uv.t;

        gl_FragColor = texture2D(textureL, uv);
    } else {
        gl_FragColor = vec4(0.0);
    }
}

