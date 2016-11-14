uniform sampler2D texture;

varying vec2 uvRed;
varying vec2 uvGreen;
varying vec2 uvBlue;

void main() {
    gl_FragColor =  vec4(texture2D(texture, uvRed).x, texture2D(texture, uvGreen).y, texture2D(texture, uvBlue).z, 1.0);
}
