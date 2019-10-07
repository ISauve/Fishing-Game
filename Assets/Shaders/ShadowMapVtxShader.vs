#version 330

// Input attributes ---------------
in vec3 position;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Main function ---------------
void main() {
    gl_Position = Projection * View * Model * vec4(position, 1.0);
}
