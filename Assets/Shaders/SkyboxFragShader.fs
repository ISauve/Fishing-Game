#version 330

// Input attributes ---------------
in vec3 texCoords;

// Input uniforms ---------------
uniform samplerCube DaySkybox;
uniform samplerCube NightSkybox;
uniform float BlendFactor;

// Output data ---------------
out vec4 fragColour;

// Main function ---------------
void main() {
    fragColour = mix(texture(DaySkybox, texCoords),
                     texture(NightSkybox, texCoords),
                     BlendFactor);
}
