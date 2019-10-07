#version 330

// Input attributes ---------------
in vec3 position;
in vec3 normal;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Output data ---------------
out vec4 clipSpaceCoords;   // for projective texture mapping
out vec2 textureCoords;     // for the bump & normal maps
out vec3 worldPosition;

// Main function ---------------
void main() {
    gl_Position = Projection * View * Model * vec4(position, 1.0);
    
    clipSpaceCoords = Projection * View * Model * vec4(position, 1.0);
    worldPosition = vec3(Model * vec4(position, 1.0));
    
    // We'll apply a tiling factor to change the texture coordinate range from [0, 1] to [0, tilingFactor]
    // -> bc we have GL_REPEAT on, this will have the effect of fitting tilingFactor "tiles" into the space
    // where 1 square of the texture should fit
    float tilingFactor = 20.0f;
    
    // Recall: the water primitive is a square of size 2 lying on the X axis, centered at the origin
    textureCoords = vec2((position.x + 1.0f) / 2.0f, (position.z + 1.0f) / 2.0f) * tilingFactor;
}

