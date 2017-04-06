attribute highp vec3 vertex;
attribute highp vec2 uv;

uniform mat4 cameraMatrix;
uniform float scale;

varying highp vec2 texCoord;

void main() {
    gl_Position = cameraMatrix * vec4(vertex * scale, 1.0);
    texCoord = uv;
}

