#version 330

// Input attributes ---------------
in vec4 clipSpaceCoords;
in vec2 textureCoords;
in vec3 worldPosition;

// Input uniforms ---------------
uniform vec3 LightColor;
uniform vec3 LightPosition;
uniform vec3 CameraPosition;

uniform sampler2D ReflectionTexture;
uniform sampler2D RefractionTexture;
uniform sampler2D RefractionDepthTexture;

uniform int Mode; // 0 = regular, 1 = reflection, 2 = refraction, 3 = bump mapping

uniform float Near;
uniform float Far;

struct WaveMap {
    sampler2D DisplacementMap;  // DuDv map
    sampler2D BumpMap;          // Normal map
};
uniform WaveMap wavemap;
uniform float Time;
uniform float WaterDistortion;
uniform bool BumpMapping;

struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float shininess;
};
uniform Material material;

// Output data ---------------
out vec4 fragColour;

// Helper functions ---------------
vec2 getProjectiveTexCoords()
{
    /*
     Projective texture mapping
     We want to calculate texture coordinates for our reflective & refractive textures
     We can do this by converting the position to normalized device coordinates ( = [-1, 1] ) & then to the texture coords ( = [0, 1] )
     */
    vec3 NDC = clipSpaceCoords.xyz / clipSpaceCoords.w;
    vec2 projectiveTexCoords = vec2((NDC.x + 1.0f) / 2.0f, (NDC.y + 1.0f) / 2.0f);
    
    return projectiveTexCoords;
}

float getWaterDepth(vec2 textureCoords)
{
    // First thing we want is the distance from the camera to the bottom of the lake (under the point we're shading)
    // We can get this value from the refraction texture depth buffer
    float refractiveDepthBufferVal = texture(RefractionDepthTexture, textureCoords).r;
    
    // The value above is a relative distance value between 0-1, but we want the absolute distance from the camera to that point
    // We can use the near & far clip planes from the camera's projection matrix to find this "true z" value
    // SOURCE: https://stackoverflow.com/questions/6652253/getting-the-true-z-value-from-the-depth-buffer
    refractiveDepthBufferVal = refractiveDepthBufferVal * 2.0f - 1.0f; // transform from [0, 1] -> [-1, 1]
    float trueTerrainZ = 2.0 * Near * Far / (Near + Far - refractiveDepthBufferVal * (Far - Near));
    
    // To get the depth of the water, we'll now need the distance from the camera to the water pixel we're shading
    float waterDepthBufferVal = gl_FragCoord.z; // stores the relative depth of the pixel we're shading
    
    // Once again, we need to transform from a relative [0, 1] value to the absolute distance
    waterDepthBufferVal = waterDepthBufferVal * 2.0f - 1.0f;
    float trueWaterZ = 2.0 * Near * Far / (Near + Far - waterDepthBufferVal * (Far - Near));
    
    return trueTerrainZ - trueWaterZ; // Water depth is simply distance between water & the bottom of the terrain
}

vec2 getWaveMapTexCoords()
{
    /*
     In order to give the water a textured look, we're going to sample a displacement & bump map
     To make the surface of the water appear to move, we're going to apply an offset to both
     of these texture lookups which will vary with time
     This function calculates the distorted texture coordinate which will be used to sample both maps
     SOURCE: https://www.youtube.com/watch?v=7T5o4vZXAvI&list=PLRIWtICgwaX23jiqVByUs0bqhnalNTNZh&index=7
     */

    vec2 distortedCoords = texture(wavemap.DisplacementMap, vec2(textureCoords.x + Time, textureCoords.y)).rg * 0.1;
    distortedCoords = textureCoords + vec2(distortedCoords.x, distortedCoords.y + Time);
    
    return distortedCoords;
}

vec2 getDisplacementDistortion(float waterDepth, vec2 waveMapTexCoords)
{
    /*
     Displacement mapping
     We want to perturb the projective texture coords which will be used to sample the reflective/refractive textures
     in order to give the water an "uneven" appearance. This function calculates the displacement distortion (which we
     will apply to the texture coordinates) by sampling the displacement map.
     */
    vec2 displacement = texture(wavemap.DisplacementMap, waveMapTexCoords).rg;
    displacement = displacement * 2.0f - 1.0f;  // Shift from [0, 1] -> [-1, 1]
    
    // If we're near the edge of the water, we want the distortion to decrease (eventually to 0)
    // This prevents a bug where we displace our projective texture coordinates so much that at the edge of the water
    // on the refraction texture we actually sample the sky instead of the terrain (-> weird discolorations)
    if (waterDepth < 20.0f) displacement *= waterDepth / 20.0f;
    
    // Scale the magnitude of the displacement according to the given WaterDistortion value
    if (WaterDistortion == 0.0f)
        displacement = vec2(0.0f, 0.0f);
    else
        displacement /= (1.0f - WaterDistortion) * 180.0f + 20.0f; // Dampener ranges from [200, 20]
    
    return displacement;
}

vec3 getPerturbedNormal(vec2 waveMapTexCoords)
{
    if (!BumpMapping) return vec3(0, 1, 0); // do not perturb the normal if bump mapping is turned off
    /*
     Bump mapping
     We're going to perturb the normals of the water pixels in order to make it appear to have a "bumpy" surface
     Since water normal is always (0, 1, 0), we'll get a perturbed normal by sampling a bump map
     */
    vec3 perturbedNormal = texture(wavemap.BumpMap, waveMapTexCoords).rgb;
    
    // At this point our normals has only values in [0,1] -> we'll change x & z to have a range of [-1, 1]
    perturbedNormal = vec3(perturbedNormal.r * 2.0f - 1.0f,
                           perturbedNormal.b * 3.5f,    // Make Y a little longer so it dominates
                           perturbedNormal.g * 2.0f - 1.0f);
    
    return normalize(perturbedNormal);
}

vec4 getSpecularHighlights(float waterDepth, vec3 surfaceNormal)
{
    // Use classic Phong lighting to calculate specular component
    vec3 v = normalize(CameraPosition - worldPosition);
    vec3 l = normalize(worldPosition - LightPosition);
    vec3 reflectedLight = reflect(l, surfaceNormal);
    float specImpact = max(dot(v, reflectedLight), 0.0);
    vec3 specular = material.ks * pow(specImpact, material.shininess) * LightColor;
    
    // Just like we did for the displacement, we'll dampen the specular highlights according to water depth
    if (waterDepth < 10.0f) specular *= waterDepth / 10.0f;

    return vec4(specular, 0.0f);
}

float getBlendFactor(vec3 surfaceNormal)
{
    /*
     Fresnel effect
     In order to determine how to blend our reflection & refraction samples, we'll determine the angle at which we
     are viewing the water's surface and sample more reflection when we're lower & more refraction when we're higher
     */
    float blendFactor = dot(normalize(CameraPosition - worldPosition), surfaceNormal);
    blendFactor = pow(blendFactor, 0.4);        // make the water less reflective in general
    blendFactor = clamp(blendFactor, 0.0, 1.0); // clamp to a valid range
    return blendFactor;
}

// Main function ---------------
void main()
{
    // Get the projective texture coordinates of the pixel we're shading (texture lookup coordinates)
    vec2 projectiveTexCoords = getProjectiveTexCoords();
    
    // Get the water depth at this pixel
    float waterDepth = getWaterDepth(projectiveTexCoords);
    
    // Get distorted bump/displacement map texture coordinates
    vec2 waveMapTexCoords = getWaveMapTexCoords();
    
    // Perturb our texture coordinates in order to distort the sampling a little
    vec2 distortion = getDisplacementDistortion(waterDepth, waveMapTexCoords);
    projectiveTexCoords += distortion;
    projectiveTexCoords.x = clamp(projectiveTexCoords.x, 0.001, 0.999);   // Clamp to a valid range
    projectiveTexCoords.y = clamp(projectiveTexCoords.y, 0.001, 0.999);
    
    // Sample our reflection & refraction textures
    // NOTE: this is where he added other stuff
    vec4 refractionColor = texture(RefractionTexture, projectiveTexCoords);
    vec4 reflectionColor = texture(ReflectionTexture,
                                   vec2(projectiveTexCoords.x, -projectiveTexCoords.y)); // flip y bc it's a reflection
    
    // Early out if we're in a special rendering mode
    switch (Mode) {
        case 0: break;  // regular
            
        case 1:         // reflection
            fragColour = reflectionColor;
            return;
            
        case 2:        // refraction
            fragColour = mix(refractionColor, vec4(0.0, 0.0, 0.0, 1.0), 0.2); // mix in a little black to make it clear there's a difference
            return;
            
        case 3:        // bump mapping
            fragColour = texture(wavemap.BumpMap, waveMapTexCoords);
            return;
            
        default: break;
    }
    
    // Get the surface normal (via bump mapping)
    vec3 perturbedNormal = getPerturbedNormal(waveMapTexCoords);
    
    // Get the specular highlights
    vec4 specularHighlights = getSpecularHighlights(waterDepth, perturbedNormal);
    
    // Get the Fresnel blend factor & use it to mix the reflective/refractive colors + dampen the specular highlights
    float blendFactor = getBlendFactor(perturbedNormal);
    fragColour = mix(reflectionColor, refractionColor + specularHighlights, blendFactor);

    // Add a bit of "ocean boat blue"
    fragColour = mix(fragColour, vec4(0, 0.467, 0.745, 1.0), 0.1);
    
    // At the edge of the lake, we want the water to fade into the terrain smoothly, so we'll decrease the fragment's
    // alpha value as we approach the edge
    if (waterDepth < 20.0f) fragColour.a = waterDepth / 20.0f; // goes from 1 to 0
}
