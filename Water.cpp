#include "Object.hpp"
#include "Scene.hpp"
#include <string>

using namespace std;
using namespace glm;

Water::Water(ShaderProgram* shader, Scene* scene) : Object(shader, scene),
    m_renderingMode(REGULAR), m_distortion(0.63f), m_bumpMapping(true)
{
    // VAO is already bound
    m_shader->enable();
    
    // Water primitive is just a square
    GLfloat positions[] = {
        1.0f, 0.0f,  1.0f,   // Front right
        1.0f, 0.0f, -1.0f,   // Back right
        -1.0f, 0.0f, -1.0f,  // Back left
        -1.0f, 0.0f,  1.0f,  // Front left
    };
    m_vbo = storeToVBO(positions, sizeof(positions));
    
    GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
    };
    m_ebo = storeToEBO(indices, sizeof(indices));
    
    // Load the du/dv map into texture unit 3 (units 0-2 are for the reflection/refraction textures)
    glActiveTexture(GL_TEXTURE3);
    m_textureIDs.push_back( storeTex("Assets/Water/dudvMap.png", GL_REPEAT) );
    
    // Load the normal map into texture unit 4
    glActiveTexture(GL_TEXTURE4);
    m_textureIDs.push_back( storeTex("Assets/Water/normalMap.png", GL_REPEAT) );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex positions
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);

    //      2) The reflection texture uniform
    glActiveTexture(GL_TEXTURE0);
    location = m_shader->getUniformLocation("ReflectionTexture");
    glUniform1i(location, 0);   // texture unit 0
    
    //      3) The refraction texture uniform
    glActiveTexture(GL_TEXTURE1);
    location = m_shader->getUniformLocation("RefractionTexture");
    glUniform1i(location, 1);   // texture unit 1
    
    //      4) The refraction depth texture uniform
    glActiveTexture(GL_TEXTURE2);
    location = m_shader->getUniformLocation("RefractionDepthTexture");
    glUniform1i(location, 2);   // texture unit 2
    
    //      5) The du/dv map texture uniform
    glActiveTexture(GL_TEXTURE3);
    location = m_shader->getUniformLocation("wavemap.DisplacementMap");
    glUniform1i(location, 3);   // texture unit 3
    
    //      6) The normal map texture uniform
    glActiveTexture(GL_TEXTURE4);
    location = m_shader->getUniformLocation("wavemap.BumpMap");
    glUniform1i(location, 4);   // texture unit 4
    
    //      7) The projection matrix near/far values
    location = m_shader->getUniformLocation("Near");
    glUniform1f(location, m_scene->camera()->near());
    location = m_shader->getUniformLocation("Far");
    glUniform1f(location, m_scene->camera()->far());
    
    uploadMaterialUniforms(vec3(0.0, 0.0, 0.0), // kd
                           vec3(0.6, 0.6, 0.6), // ks
                           20);                 // shininess
    
    m_shader->disable();
    glBindVertexArray(0);
    releaseData();
}

void Water::uploadLightingUniforms()
{
    GLint location = m_shader->getUniformLocation("LightColor");
    glUniform3fv( location, 1, value_ptr(m_scene->sun()->color()) );
    
    location = m_shader->getUniformLocation("LightPosition");
    glUniform3fv( location, 1, value_ptr(m_scene->sun()->position()) );
    
    location = m_shader->getUniformLocation("CameraPosition");
    glUniform3fv(location, 1, value_ptr(m_scene->camera()->position()));
    
    // No ambient intensity
    
    CHECK_GL_ERRORS;
}

void Water::uploadClippingUniforms(Mode m)
{
    // water shader doesn't take clipping uniforms
}

static float TIME = 0.0f; // slowly goes from 0->1 repeatedly
void Water::uploadCustomUniforms(Mode m)
{
    // Pass the "Time" value
    GLint location = m_shader->getUniformLocation("Time");
    glUniform1f(location, TIME);
    TIME += 0.001f;
    if (TIME > 1.0f) TIME -= 1.0f;
    
    location = m_shader->getUniformLocation("WaterDistortion");
    glUniform1f(location, m_distortion);
    
    location = m_shader->getUniformLocation("BumpMapping");
    glUniform1i(location, m_bumpMapping);
    
    location = m_shader->getUniformLocation("Mode");
    glUniform1i(location, (int) m_renderingMode);
    
    CHECK_GL_ERRORS;
}

void Water::bindData()
{
    glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );
    
    // We need to upload the reflection & refraction textures stored in their respective FBOs
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene->reflectionTexture());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_scene->refractionTexture());
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_scene->refractionDepthTexture());
    
    // Bind the normal & du/dv maps regularly
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[0]);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[1]);
    
    CHECK_GL_ERRORS;
}

void Water::drawElements()
{
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    
    CHECK_GL_ERRORS;
}

void Water::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}
