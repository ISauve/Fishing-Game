#version 330

// Input attributes ---------------
in vec3 position;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Output data ---------------
out vec3 texCoords;

// Main function ---------------
void main() {
	gl_Position = Projection * View * Model * vec4(position, 1.0);

    // Sample the positions of the cube as texture coordinates
    texCoords = position;
}
