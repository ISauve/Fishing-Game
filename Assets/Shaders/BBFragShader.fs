#version 330

// Input attributes ---------------
in vec3 worldPosition;

// Output data ---------------
out vec4 fragColour;

// Main function ---------------
void main()
{
    fragColour = vec4(1, 0, 0.5, 0.5);
}
