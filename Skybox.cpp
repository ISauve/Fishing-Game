#include "Object.hpp"
#include "Scene.hpp"
#include <string>

using namespace std;
using namespace glm;

Skybox::Skybox(ShaderProgram* shader, Scene* scene, float speed) : Object(shader, scene), m_rotationSpeed(speed), m_isDay(true)
{
    // VAO is already bound
    m_shader->enable();
    
    // Vertex data simply represents a large cube
    GLfloat points[] = {
        1.0f,  1.0f, 1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  -1.0f, 1.0f,
        1.0f,  -1.0f, -1.0f,
        -1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  -1.0f, 1.0f,
        -1.0f,  -1.0f, -1.0f
    };
    m_vbo = storeToVBO(points, sizeof(points));
    
    GLuint indices[] = {
        5, 7, 3,
        3, 1, 5,
        
        6, 7, 5,
        5, 4, 6,
        
        3, 2, 0,
        0, 1, 3,
        
        6, 4, 0,
        0, 2, 6,
        
        5, 1, 0,
        0, 4, 5,
        
        7, 6, 3,
        3, 6, 2
    };
    m_ebo = storeToEBO(indices, sizeof(indices));
    
    // Load the cubemap day textures into texture unit 0
    vector<string> dayFaces {
        "Assets/Skybox/right.png",
        "Assets/Skybox/left.png",
        "Assets/Skybox/top.png",
        "Assets/Skybox/bottom.png",
        "Assets/Skybox/front.png",
        "Assets/Skybox/back.png"
    };
    glActiveTexture(GL_TEXTURE0);
    m_textureIDs.push_back( storeCubeMap(dayFaces) );
    
    // Load the cubemap night textures into texture unit 1
    vector<string> nightFaces {
        "Assets/Skybox/nightRight.png",
        "Assets/Skybox/nightLeft.png",
        "Assets/Skybox/nightTop.png",
        "Assets/Skybox/nightBottom.png",
        "Assets/Skybox/nightFront.png",
        "Assets/Skybox/nightBack.png"
    };
    glActiveTexture(GL_TEXTURE1);
    m_textureIDs.push_back( storeCubeMap(nightFaces) );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex data
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);

    //      2) The first samplerCube uniform
    location = m_shader->getUniformLocation("DaySkybox");
    glUniform1i(location, 0);   // texture unit 0
    
    //      3) The second samplerCube uniform
    location = m_shader->getUniformLocation("NightSkybox");
    glUniform1i(location, 1);   // texture unit 1
    
    m_shader->disable();
    glBindVertexArray(0);
    releaseData();
}

void Skybox::uploadLightingUniforms()
{
    // No-op, no lighting needed for skybox
}

void Skybox::uploadModelUniform()
{
    // Increase the rotation
    float FPS = 60.0f; // currently hard programmed
    m_rotation.y = m_rotation.y - m_rotationSpeed / FPS;
    
    // Call the base class version to finish the job
    Object::uploadModelUniform();
}

void Skybox::uploadViewUniform()
{
    // Remove the translation component of the view matrix
    mat4 view = mat4(mat3(m_scene->camera()->viewMatrix()));
    GLint location = m_shader->getUniformLocation("View");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(view));
    
    CHECK_GL_ERRORS;
}

void Skybox::uploadClippingUniforms(Mode m)
{
    // skybox shader doesn't take clipping uniforms
}

void Skybox::uploadCustomUniforms(Mode m)
{
    // Determine & upload the blend factor uniform
    float blendFactor = (m_isDay) ? 0.0f : 1.0f;    
    if (m_rotation.y < -360) {        // After 1 rotation: switch completely to the next texture
        m_isDay = !m_isDay;
        blendFactor = (m_isDay) ? 0.0f : 1.0f;
        m_rotation.y += 360;
    }
    else if (m_rotation.y < -270) {   // After 0.75 rotations: start blending
        if (m_isDay)    blendFactor = 1.0f - (m_rotation.y + 360) / 90.0f;
        else            blendFactor = (m_rotation.y + 360) / 90.0f;
    }
    
    GLint location = m_shader->getUniformLocation("BlendFactor");
    glUniform1f(location, blendFactor);
    
    CHECK_GL_ERRORS;
}

void Skybox::bindData()
{
    glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureIDs[0]);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureIDs[1]);
    
    CHECK_GL_ERRORS;
}

void Skybox::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}

void Skybox::drawElements()
{
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    
    CHECK_GL_ERRORS;
}


