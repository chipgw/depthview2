uniform sampler2D textureL;
uniform sampler2D textureR;

varying highp vec2 texCoord;

uniform bool mirrorL;
uniform bool mirrorR;

void main(void) {
    texCoord.s *= 2.0;
    if (texCoord.s > 1.0) {
        texCoord.s -= 1.0;
        if(mirrorR)
            texCoord.s = 1.0 - texCoord.s;

        gl_FragColor = texture2D(textureR, texCoord);
    } else if (texCoord.s < 1.0) {
        if(mirrorL)
            texCoord.s = 1.0 - texCoord.s;

        gl_FragColor = texture2D(textureL, texCoord);
    } else {
        gl_FragColor = vec4(0.0);
    }
}

