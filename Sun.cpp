#include "Object.hpp"
#include "Scene.hpp"

using namespace std;
using namespace glm;

Sun::Sun(ShaderProgram* shader, Scene* scene) : Object(shader, scene),
    m_color(vec3(1.0, 1.0, 1.0)), m_directionToSun(0.0f),
    m_dist(10000.0f) // make it really far so the lighting doesn't change drastically as the character moves
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
    m_textureIDs.push_back( storeTex("Assets/sun.png") );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex data
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    
    //      2) The sun texture uniform
    location = m_shader->getUniformLocation("Image");
    glUniform1i(location, 0);   // texture unit 0
    
    m_shader->disable();
    glBindVertexArray(0);
    releaseData();
}

void Sun::uploadLightingUniforms()
{
    // no-op, no lighting needed for sun
}

mat4 Sun::modelMatrix()
{
    // For the purposes of rendering the sun, we want the texture to be relative to the
    // location of the camera.  This is done so that the sun always appears in the same
    // spot -> it looks like it's part of the skybox
    updateDirectionToSun();
    vec3 position = m_scene->camera()->position() + m_dist * m_directionToSun;
    
    // We want to remove any rotational component from our final MVP matrix in order
    // to have the sun always appear to face the camera directly. In order to do that,
    // we have to make a model matrix which cancels out any rotational component of the
    // view matrix - which we can do by making the upper 3x3 portion be the transpose
    // of the upper3x3 of the view matrix
    mat4 view = m_scene->camera()->viewMatrix();
    mat4 model = translate(mat4(1.0f), position); // start off with the translational component
    model[0][0] = view[0][0];
    model[0][1] = view[1][0];
    model[0][2] = view[2][0];
    model[1][0] = view[0][1];
    model[1][1] = view[1][1];
    model[1][2] = view[2][1];
    model[2][0] = view[0][2];
    model[2][1] = view[1][2];
    model[2][2] = view[2][2];
    
    // Finally, we can apply the scaling transformation
    model = scale(model, vec3(m_size, m_size, m_size));
    
    return model;
}

void Sun::uploadClippingUniforms(Mode m)
{
    // sun shader doesn't take clipping uniforms
}

void Sun::uploadCustomUniforms(Mode m)
{
    GLint location = m_shader->getUniformLocation("Transparency");
    glUniform1f(location, 1.0f);
    
    CHECK_GL_ERRORS;
}

void Sun::bindData()
{
    glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[0]);
    
    CHECK_GL_ERRORS;
}

void Sun::drawElements()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    CHECK_GL_ERRORS;
}

void Sun::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}

void Sun::updateDirectionToSun()
{
    // We'll vary the "direction to camera" vector with time to give the effect the the sun
    // rises & sets throughout the day
    float time = m_scene->skybox()->time();
    
    // The sun goes from a height of -0.3 to 0.8 and back throughout the day
    //      Note: the -0.3 is so that the full sun is below the horizon
    if (time < 0.5)
        m_directionToSun.y = time * 2.2 - 0.3f;
    else
        m_directionToSun.y = (time - 1.0f) * -2.2f - 0.3f;
    
    if (m_scene->skybox()->isDay()) {
        // The sun goes from right (x = -1.5) to left (x = 1.5) throughout the day
        m_directionToSun.x = time * 3.0f - 1.5f;
    } else {
        // Same idea as the day, except we'll make the sun go from left to right
        // Note that the reason this is still being updated when the sun isn't showing is
        // so that the lighting in the scene changes smoothly (it's still the light source)
        m_directionToSun.x = (1.0f - time) * 3.0f - 1.5f;
    }
    
    m_directionToSun.z = 1.0f; // constant
    
    m_directionToSun = normalize(m_directionToSun);
}

void Sun::updateClippingPlanes()
{
    // For now, we'll just hardcode a clipping box that encompasses all the objects
    m_left = -350.0f;
    m_right = 450.0f;
    m_top = 200.0f;
    m_bottom = -100.0f;
    m_near = m_dist - 500.0f;
    m_far = m_dist + 500.0f;
    
    // In the future, we'll want to update this so that the clipping planes form a MINIMAL box
    // which covers everything the camera can see (to a reasonable distance)
    // ----- TODO
}

mat4 Sun::viewMatrix()
{
    updateDirectionToSun();
    return lookAt(position(), vec3(0.0), vec3(0, 1, 0)); // look at the center of the terrain
}

mat4 Sun::orthographicProjMatrix()
{
    updateClippingPlanes();
    return ortho(m_left, m_right, m_bottom, m_top, m_near, m_far);
}

