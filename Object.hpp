#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/GlErrorCheck.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "Renderable.hpp"

class Scene;
class Model;

/***********************************************************
                    Abstract Base Class
 ***********************************************************/

class Object : public Renderable {
protected:
    // Pointer to the scene in order to access the camera, light source, terrain, etc
    Scene* m_scene;
    
    ShaderProgram* m_shader;
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    std::vector <GLuint> m_textureIDs;
    
    glm::vec3 m_position;
    float m_size;
    glm::vec3 m_rotation;  // in degrees, angle per axis
    
    // Helpers for binding data to buffers
    GLuint storeToVBO(GLfloat* vertices, int size);
    GLuint storeToVBO(GLfloat* positions, int sizeP, GLfloat* normals, int sizeN, GLfloat* texCoords, int sizeT);
    GLuint storeToEBO(GLuint* indices, int size);
    GLuint storeTex(std::string path, GLenum wrapping = GL_REPEAT);
    GLuint storeCubeMap(std::vector<std::string>& faces);
    
    // Template method pattern: the following helper functions can be overridden
    // by derived classes to customize the rendering of the objects
    virtual void uploadLightingUniforms();
    virtual void uploadMaterialUniforms(glm::vec3 kd, glm::vec3 ks, float shininess);
    virtual void uploadModelUniform();
    virtual void uploadViewUniform();
    virtual void uploadProjectionUniform();
    virtual void uploadClippingUniforms(Mode m);
    virtual void uploadCustomUniforms(Mode m);
    virtual void bindData();
    virtual void drawElements();
    virtual void releaseData();
    
public:
    Object(ShaderProgram* shader, Scene* scene);
    virtual ~Object();
    
    void render(Mode m) final;
    virtual void renderToShadowMap() {};
    
    virtual glm::vec3 position()    { return m_position; }; // virtual bc Sun overrides it
    virtual glm::mat4 modelMatrix();                        // public bc Character accesses it
    
    void setPosition(glm::vec3 p)   { m_position = p; };
    void setSize(float s)           { m_size = s; };
    void setRotation(glm::vec3 r)   { m_rotation = r; };
    void rotate(glm::vec3 rotation) { m_rotation += rotation; };
};

/***********************************************************
                        Derived Classes
 ***********************************************************/

class Skybox : public Object {
    float m_rotationSpeed;    // degrees per second
    bool m_isDay;
    
    // Overridden template methods
    void uploadLightingUniforms() override;
    void uploadModelUniform() override;
    void uploadViewUniform() override;
    void uploadClippingUniforms(Mode m) override;
    void uploadCustomUniforms(Mode m) override;
    void bindData() override;
    void drawElements() override;
    void releaseData() override;
    
public:
    Skybox(ShaderProgram* shader, Scene* scene, float rotateSpeed);
    
    void setRotationSpeed(float r) { m_rotationSpeed = r; };
    
    bool isDay() { return m_isDay; };
    float time() { return m_rotation.y / -360.0f; }; // Returns a value from 0-1 indicating how far
                                                     // though the current time (day or night) we are
};

// ------------------------------

class Water : public Object {
    float m_distortion;
    bool m_bumpMapping;
    Mode m_renderingMode;
    
    // Overridden template methods
    void uploadLightingUniforms() override;
    void uploadClippingUniforms(Mode m) override;
    void uploadCustomUniforms(Mode m) override;
    void bindData() override;
    void drawElements() override;
    void releaseData() override;
    
public:
    Water(ShaderProgram* shader, Scene* scene);
    
    void setDistortion(float d) { m_distortion = d; };
    void setBumpMapping(bool b) { m_bumpMapping = b; };
    void setMode(Mode m)        { m_renderingMode = m; };
};

// ------------------------------

class Terrain : public Object {
    float m_size;       // size of one side of the terrain square
    float m_maxHeight;  // height of the peaks
    int m_texture;
    int m_numIndices;
    unsigned m_heightMapSize;
    
    // Calculate these at initialization
    std::vector< std::vector<float> > m_heights;
    std::vector< std::vector<glm::vec3> > m_normals;
    
    void calculateHeightsAndNormals(unsigned char* heightMap);
    float baryCentric(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec2 pos);
    
    // Overridden template methods
    void uploadCustomUniforms(Mode m) override;
    void bindData() override;
    void drawElements() override;
    void releaseData() override;

public:
    Terrain(ShaderProgram* shader, Scene* scene, float size, float maxHeight);
    
    float getHeightAt(float x, float z);
    float getSize() { return m_size; };
};

// ------------------------------

// The sun is a 2D texture which is rendered in front of the skybox
// It also acts as the light source for the scene
class Sun : public Object {
    glm::vec3 m_color;
    glm::vec3 m_directionToSun;
    float m_dist;
    
    // Shadow map clipping planes
    float m_left;
    float m_right;
    float m_top;
    float m_bottom;
    float m_near;
    float m_far;
    
    void updateDirectionToSun();
    void updateClippingPlanes();
    glm::vec3 clippingBoxCenter();
  
    // Overridden template methods
    void uploadLightingUniforms() override;
    void uploadClippingUniforms(Mode m) override;
    void uploadCustomUniforms(Mode m) override;
    void bindData() override;
    void drawElements() override;
    void releaseData() override;
    
public:
    Sun(ShaderProgram* shader, Scene* scene);
    
    glm::vec3 color()               { return m_color; };
    glm::vec3 position() override   { return m_dist * m_directionToSun; };
    glm::mat4 modelMatrix() override;
    
    // View & projection matrix from the Sun's perspective are for shadow mappings
    glm::mat4 viewMatrix();
    glm::mat4 orthographicProjMatrix();
};

// ------------------------------

// An Image2D is a 2D GUI image which is rendered directly to the screen (2D space, not 3D space)
class Image2D : public Object {
    glm::vec2 m_size2D;
    float m_transparency;
    
    // Overridden template methods
    void uploadLightingUniforms() override;

    void uploadViewUniform() override;
    void uploadProjectionUniform() override;
    void uploadClippingUniforms(Mode m) override;
    void uploadCustomUniforms(Mode m) override;
    void bindData() override;
    void drawElements() override;
    void releaseData() override;
    
public:
    Image2D(ShaderProgram* shader, Scene* scene, std::string path, glm::vec2 size);
    
    glm::mat4 modelMatrix() override;
    
    void setImage(std::string path);
    void setTransparency(float t) { m_transparency = t; };
};

// ------------------------------

class Mesh : public Object {
    friend class Model; // a container of Meshes
    
    int m_numIndices;
    
    // Overridden template methods
    void bindData() override;
    void drawElements() override;
    void releaseData() override;
    void uploadCustomUniforms(Mode m) override;
    
    // Bounding box information (for collision detection)
    glm::vec3 m_minBounds;
    glm::vec3 m_maxBounds;
    GLuint bb_vao;
    GLuint bb_vbo;
    ShaderProgram bb_shader;
    
    void initBoundingBoxData();
    void renderBoundingBox(Mode m);
    
public:
    Mesh(ShaderProgram* shader, Scene* scene, aiMesh* mesh, const aiScene* aiscene, std::string texturePrefix);
    ~Mesh();
    
    void renderToShadowMap() final;
    
    bool collision(Mesh* m); // checks if the bounding boxes of these meshes collide
    
    glm::vec3 minBounds() { return m_minBounds; };
    glm::vec3 maxBounds() { return m_maxBounds; };
};
