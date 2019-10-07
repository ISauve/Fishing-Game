#version 330

// Input attributes ---------------
in vec3 position;
in vec3 normal;
in vec2 textureCoords;

// Input uniforms ---------------
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform mat4 ToShadowMapSpace;

uniform bool ClippingEnabled;
uniform vec4 ClippingPlane;

// Output data ---------------
out vec3 worldPosition;
out vec3 surfaceNormal; // normalized
out vec2 texCoords;
out vec4 shadowMapPosition;

// Main function ---------------
void main() {
    gl_Position = Projection * View * Model * vec4(position, 1.0);
    
    worldPosition = vec3(Model * vec4(position, 1.0));
    surfaceNormal = normalize(vec3(mat3(transpose(inverse(Model))) * normal));
    texCoords = textureCoords; // simply pass this along
    shadowMapPosition = ToShadowMapSpace * vec4(worldPosition, 1.0f);
    
    if (ClippingEnabled) gl_ClipDistance[0] = dot(vec4(worldPosition, 1), ClippingPlane);
    else                 gl_ClipDistance[0] = 1; // don't clip
}
