#include "Object.hpp"
#include "Scene.hpp"

using namespace std;
using namespace glm;

Image2D::Image2D(ShaderProgram* shader, Scene* scene, string path, vec2 size) :
    Object(shader, scene), m_size2D(size), m_transparency(1.0f)
{
    // VAO is already bound
    m_shader->enable();
    
    // Vertex data is simply a 2D quad
    GLfloat points[] = {
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f
    };
    
    m_vbo = storeToVBO(points, sizeof(points));
    
    // Load the sun texture
    glActiveTexture(GL_TEXTURE0);
    m_textureIDs.push_back( storeTex("Assets/" + path) );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex data
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    
    //      2) The texture uniform
    location = m_shader->getUniformLocation("Image");
    glUniform1i(location, 0);   // texture unit 0
    
    m_shader->disable();
    glBindVertexArray(0);
    releaseData();
}

void Image2D::setImage(string path)
{
    m_textureIDs.clear();
    
    glBindVertexArray(m_vao);
    m_shader->enable();
    
    // Load the new texture in texture unit 0
    glActiveTexture(GL_TEXTURE0);
    m_textureIDs.push_back( storeTex("Assets/" + path) );
    
    m_shader->disable();
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    CHECK_GL_ERRORS;
}

void Image2D::uploadLightingUniforms()
{
    // no-op, no lighting needed
}

mat4 Image2D::modelMatrix()
{
    mat4 model = translate(mat4(1.0f), m_position) *
                 scale(mat4(1.0f), vec3(m_size2D.x, m_size2D.y, 1.0f));
    return model;
}

void Image2D::uploadViewUniform()
{
    // Don't need a real view matrix bc we want the image to render directly onto the screen
    // (not in 3D space, but 2D)
    mat4 view = mat4(1.0f);
    GLint location = m_shader->getUniformLocation("View");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(view));
    
    CHECK_GL_ERRORS;
}


void Image2D::uploadProjectionUniform()
{
    // We also don't need a projection matrix
    mat4 proj = mat4(1.0f);
    GLint location = m_shader->getUniformLocation("Projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(proj));
    
    CHECK_GL_ERRORS;
}

void Image2D::uploadClippingUniforms(Mode m)
{
    // image shader doesn't take clipping uniforms
}

void Image2D::uploadCustomUniforms(Mode m)
{
    GLint location = m_shader->getUniformLocation("Transparency");
    glUniform1f(location, m_transparency);
    
    CHECK_GL_ERRORS;
}

void Image2D::bindData()
{
    glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[0]);
    
    CHECK_GL_ERRORS;
}

void Image2D::drawElements()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Image2D::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}

