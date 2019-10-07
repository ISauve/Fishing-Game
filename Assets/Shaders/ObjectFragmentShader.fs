#version 330

// Input attributes ---------------
in vec3 worldPosition;
in vec3 surfaceNormal; // normalized
in vec2 texCoords;
in vec4 shadowMapPosition;

// Input uniforms ---------------
uniform vec3 LightColor;
uniform vec3 LightPosition;
uniform vec3 CameraPosition;
uniform vec3 AmbientIntensity;

uniform bool IsTerrainObject;
uniform sampler2D GrassTexture;
uniform sampler2D DirtTexture;
uniform sampler2D ShadowMap;

uniform bool IsMeshObject;
uniform sampler2D DiffuseTexture;
//uniform sampler2D SpecularTexture;
//uniform sampler2D NormalMap;

struct Material {
    vec3 ka;
    vec3 kd;
    vec3 ks;
    float shininess;
};
uniform Material material;

// Output data ---------------
out vec4 fragColour;

// Helper function ---------------
vec4 getPhongLighting()
{
    vec3 l = normalize(LightPosition - worldPosition);
    
    /* 1) Ambient lighting */
    vec3 ambient = AmbientIntensity * LightColor;
    
    /* 2) Diffuse lighting */
    float n_dot_l = max(dot(surfaceNormal, l), 0.0); // make sure the value is not negative
    vec3 diffuse = material.kd * n_dot_l * LightColor;
    
    /* 3) Specular lighting */
    vec3 specular = vec3(0.0);
    if (n_dot_l > 0.0) {
        vec3 v = normalize(CameraPosition - worldPosition);
        vec3 h = normalize(v + l);  // halfway vector
        float n_dot_h = max(dot(surfaceNormal, h), 0.0);
        
        specular = material.ks * pow(n_dot_h, material.shininess) * LightColor;
    }
    
    return vec4(ambient + diffuse + specular, 1.0);
}

bool isOccluded()
{
    /*
     Shadow mapping
     We've rendered to the shadow map texture a map which shows the nearest distance from the light
     to an object in that pixel (from the light source's perspective). We can compare this to our
     distance to the light source in order to determine if we are the closest object - in which case
     we are NOT occluded (aka another object is not casting a shadow on us).
     */
    
    // Convert shadow map position from NDC ( [-1, 1] ) to texture coordinates ( [0, 1] )
    vec3 coords = shadowMapPosition.xyz;
    coords = (coords + 1.0f) / 2.0f;
    
    if (coords.x > 1.0f || coords.x < 0.0f || coords.y > 1.0f || coords.y < 0.0f)
        // This fragment is not on the shadow map
        return false;
    
    float nearestObjectToLightSource = texture(ShadowMap, coords.xy).r;
    float distToLightSource = coords.z;
    
    return distToLightSource > nearestObjectToLightSource;
}

// Main function ---------------
void main()
{
    if (IsTerrainObject)
    {
        vec4 lighting = getPhongLighting();
        if ( isOccluded() )
            lighting = vec4(AmbientIntensity * LightColor, 1.0f);
        
        vec4 grassColour = texture(GrassTexture, texCoords) * lighting;
        vec4 dirtColour = texture(DirtTexture, texCoords) * lighting;
        
        // Determine whether to use grass, dirt, or blend them based on position of point
        // relative to water level (y = 0)
        if (worldPosition.y > 7.0f)
            fragColour = grassColour;
        else if (worldPosition.y < -2.0f)
            fragColour = dirtColour;
        else {
            // worldPosition.y is between [-2, 7] - we'll convert it to [0, 1] for a blend factor
            float blendFactor = (worldPosition.y + 2.0f) / 9.0f;
            fragColour = mix(dirtColour, grassColour, blendFactor);
        }
    }
    else if (IsMeshObject)
    {
        vec4 lighting = getPhongLighting();
        fragColour = texture(DiffuseTexture, texCoords) * lighting;
    }
    else
    {
        fragColour = vec4(1, 0, 0, 1);
    }
}
