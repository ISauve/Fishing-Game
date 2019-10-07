#version 330

// Input attributes ---------------
in vec3 position;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform bool ClippingEnabled;
uniform vec4 ClippingPlane;

// Output data ---------------
out vec3 worldPosition;

// Main function ---------------
void main() {
    gl_Position = Projection * View * Model * vec4(position, 1.0);
    
    worldPosition = vec3(Model * vec4(position, 1.0));
    
    if (ClippingEnabled) gl_ClipDistance[0] = dot(vec4(worldPosition, 1), ClippingPlane);
    else                 gl_ClipDistance[0] = 1; // don't clip
}
