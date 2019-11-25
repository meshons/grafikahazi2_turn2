#version 330
precision highp float;

uniform vec3 wLookAt, wRight, wUp;

layout(location = 0) in vec2 cCamWindowVertex;
out vec3 p;

void main() {
    gl_Position = vec4(cCamWindowVertex, 0, 1);
    p = wLookAt + wRight * cCamWindowVertex.x + wUp * cCamWindowVertex.y;
}
