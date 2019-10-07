#version 330

// Input attributes ---------------
in vec2 position;

// Output data ---------------
out vec2 texCoords;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Main function ---------------
void main() {    
    gl_Position = Projection * View * Model * vec4(position, 0.0, 1.0);
    
    // Sample the quad position data as texture coordinates
    // Quad posns go from [-1, 1], but we want them to go from [0, 1] (w/ 0 in the upper left)
    texCoords = vec2( (position.x + 1.0f) / 2.0f,
                     1.0f - ((position.y + 1.0f) / 2.0f));
}
