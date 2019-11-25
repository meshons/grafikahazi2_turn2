#version 330				// Shader 3.3
precision highp float;		// normal floats, makes no difference on desktop computers

uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
uniform float scale;        // scale
uniform float translateX;   // x translate
layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

void main() {
    gl_Position = vec4(vp.x * scale - translateX, vp.y * scale, 0, 1) * MVP;
}
