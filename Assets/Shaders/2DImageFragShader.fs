#version 330

// Input attributes ---------------
in vec2 texCoords;

// Input uniforms ---------------
uniform sampler2D Image;
uniform float Transparency;

// Output data ---------------
out vec4 fragColour;

// Main function ---------------
void main() {
    fragColour = texture(Image, texCoords);
    fragColour.a *= Transparency;
}

