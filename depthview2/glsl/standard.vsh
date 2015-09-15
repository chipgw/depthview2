attribute highp vec4 vertex;
attribute highp vec2 uv;

varying highp vec2 texCoord;

void main() {
    gl_Position = vertex;
    texCoord = uv;
}

