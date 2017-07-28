attribute highp vec4 position;
attribute highp vec2 uvRedIn;
attribute highp vec2 uvGreenIn;
attribute highp vec2 uvBlueIn;

varying vec2 uvRed;
varying vec2 uvGreen;
varying vec2 uvBlue;

void main() {
    uvRed = uvRedIn;
    uvGreen = uvGreenIn;
    uvBlue = uvBlueIn;
    gl_Position = position;
}
