#include "Object.hpp"
#include "Scene.hpp"
#include "lodepng/lodepng.h"

using namespace std;
using namespace glm;

// Texture cache
unordered_map<string, GLuint> textureCache;

Object::Object(ShaderProgram* shader, Scene* scene) : Renderable(), m_shader(shader), m_scene(scene), m_position(vec3(0.0)), m_rotation(vec3(0, 0, 0)), m_size(1.0f)
{
    // Create & bind a vertex array object
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Note: Child class constructors get called when this returnss
};

Object::~Object()  // Note: Gets called after child class destructor is finished
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
    
    for (auto it : m_textureIDs) glDeleteTextures(1, &it);
    m_textureIDs.clear();
};

/***********************************************************
                 Binding data to buffers
 ***********************************************************/

// Create, bind, and load data into a vertex buffer object
GLuint Object::storeToVBO(GLfloat* vertices, int size)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,   // target
                 size,              // total size
                 vertices,          // data to copy over
                 GL_STATIC_DRAW);   // method
    
    CHECK_GL_ERRORS;
    return vbo;
}

// Overloaded version of the above function which loads 3 sets of data into 1 buffer
GLuint Object::storeToVBO(GLfloat* positions, int sizeP, GLfloat* normals, int sizeN, GLfloat* texCoords, int sizeT)
{
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    // We'll use glBufferSubData to load the data in 3 parts
    glBufferData(GL_ARRAY_BUFFER, sizeP + sizeN + sizeT, NULL, GL_STATIC_DRAW);
    
    glBufferSubData(GL_ARRAY_BUFFER,  // target
                    0,                // offset = none
                    sizeP,            // size
                    positions);       // data
    
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeP,            // offset = sizeof previous data entered
                    sizeN,
                    normals);
    
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeP + sizeN,            // offset = sizeof previous data entered
                    sizeT,
                    texCoords);
    
    CHECK_GL_ERRORS;
    return vbo;
}

// Create, bind, and load data into an element buffer object
GLuint Object::storeToEBO(GLuint* indices, int size)
{
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    
    CHECK_GL_ERRORS;
    return ebo;
}

// Create, bind, and load data into a 2D texture object
// Note: must set active texture unit FIRST
GLuint Object::storeTex(string path, GLenum wrapping)
{
    // Check if this particular texture has already been loaded & return its ID if so
    if (textureCache[path]) return textureCache[path];
    
    GLuint tex;
    glGenTextures(1, &tex);
    textureCache[path] = tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    
    // Load the image in 32-bit RGBA format
    unsigned char* data;
    unsigned width, height;
    unsigned error = lodepng_decode32_file(&data, &width, &height, path.c_str());
    if (error) {
        cerr << "Error decoding " << path << ". " << error << ": " << lodepng_error_text(error) << endl;
        return tex;
    }
    
    GLenum format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    free(data);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
    
    if (wrapping == GL_CLAMP_TO_BORDER) {
        // Specify a border color
        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };   // clear
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
    
    CHECK_GL_ERRORS;
    return tex;
}

// Create, bind, and load data into a texture cube map
// Note: must set active texture unit FIRST
GLuint Object::storeCubeMap(vector<string>& faces)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    
    unsigned char* data;
    unsigned width, height;
    for (int i = 0; i < faces.size(); i++) {
        // Load the image in 32-bit RGBA format
        unsigned error = lodepng_decode32_file(&data, &width, &height, faces[i].c_str());
        if (error) {
            cerr << "Error decoding " << faces[i].c_str() << ". " << error << ": " << lodepng_error_text(error) << endl;
            return tex;
        }
        
        GLenum format = GL_RGBA;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        free(data);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    CHECK_GL_ERRORS;
    return tex;
}

/***********************************************************************************************
                                    Uploading uniforms
 Note: all fcns below require the shader to have been enabled & the VAO to have been bound first
 ***********************************************************************************************/

void Object::uploadLightingUniforms()
{
    GLint location = m_shader->getUniformLocation("LightColor");
    glUniform3fv( location, 1, value_ptr(m_scene->sun()->color()) );
    
    location = m_shader->getUniformLocation("LightPosition");
    glUniform3fv( location, 1, value_ptr(m_scene->sun()->position()) );
    
    location = m_shader->getUniformLocation("CameraPosition");
    glUniform3fv(location, 1, value_ptr(m_scene->camera()->position()));
    
    location = m_shader->getUniformLocation("AmbientIntensity");
    vec3 ambientIntensity(0.5f);
    glUniform3fv(location, 1, value_ptr(ambientIntensity));
    
    CHECK_GL_ERRORS;
}

void Object::uploadMaterialUniforms(vec3 kd, vec3 ks, float shininess)
{
    GLint location = m_shader->getUniformLocation("material.kd");
    glUniform3fv(location, 1, value_ptr(kd));
    
    location = m_shader->getUniformLocation("material.ks");
    glUniform3fv(location, 1, value_ptr(ks));
    
    location = m_shader->getUniformLocation("material.shininess");
    glUniform1f(location, shininess);
    
    CHECK_GL_ERRORS;
}

void Object::uploadClippingUniforms(Mode mode)
{
    bool clippingEnabled = mode == REFLECTION || mode == REFRACTION;
    GLint location = m_shader->getUniformLocation("ClippingEnabled");
    glUniform1f(location, clippingEnabled);

    if (!clippingEnabled) return;
    
    // Define a clipping plane in the form (A, B, C, D) where A, B, C is the normal & D is the distance from the origin
    vec4 clippingPlane;
    if (mode == REFLECTION)
        // Clip everything below water level
        clippingPlane = vec4(0, 1, 0, m_scene->water()->position().y);
    
    else if (mode == REFRACTION)
        // Clip everything above water level + 1 (offset is to fix water displacement problem)
        clippingPlane = vec4(0, -1, 0, m_scene->water()->position().y + 1.0f);
    
    location = m_shader->getUniformLocation("ClippingPlane");
    glUniform4fv(location, 1, value_ptr(clippingPlane));
    
    CHECK_GL_ERRORS;
}

mat4 Object::modelMatrix()
{
    // Scale -> rotate -> translate
    mat4 model = translate(mat4(1.0f), m_position) *
                 glm::rotate(mat4(1.0f), radians(m_rotation.x), vec3(1.0f, 0.0f, 0.0f)) * // X rotation
                 glm::rotate(mat4(1.0f), radians(m_rotation.y), vec3(0.0f, 1.0f, 0.0f)) * // Y rotation
                 glm::rotate(mat4(1.0f), radians(m_rotation.z), vec3(0.0f, 0.0f, 1.0f)) * // Z rotation
                 scale(mat4(1.0f), vec3(m_size, m_size, m_size));
    return model;
}

void Object::uploadModelUniform()
{
    mat4 model = modelMatrix();
    GLint location = m_shader->getUniformLocation("Model");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    
    CHECK_GL_ERRORS;
}

void Object::uploadViewUniform()
{
    mat4 view = m_scene->camera()->viewMatrix();
    
    GLint location = m_shader->getUniformLocation("View");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(view));
    
    CHECK_GL_ERRORS;
}

void Object::uploadProjectionUniform()
{
    mat4 projection = m_scene->camera()->projMatrix();
    
    GLint location = m_shader->getUniformLocation("Projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));
    
    CHECK_GL_ERRORS;
}

void Object::uploadCustomUniforms(Mode m)
{
    
}

void Object::bindData()
{
    // must be implemented by derived classes
}

void Object::drawElements()
{
    // must be implemented by derived classes
}

void Object::releaseData()
{
    // must be implemented by derived classes
}

/***********************************************************
                        Rendering
 ***********************************************************/

void Object::render(Mode m)
{
    // Bind the object's vertex data
    glBindVertexArray(m_vao);
    m_shader->enable();
    
    // Upload all the uniforms we need
    uploadLightingUniforms();
    uploadModelUniform();
    uploadViewUniform();
    uploadProjectionUniform();
    uploadClippingUniforms(m);
    uploadCustomUniforms(m);
    
    bindData();
    drawElements();
    releaseData();
    
    m_shader->disable();
    glBindVertexArray(0);
}
