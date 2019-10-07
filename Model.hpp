#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <random>
#include "Renderable.hpp"
#include "Object.hpp"

/***********************************************************
                    Abstract Base Class
 ***********************************************************/

// A model is simply a container of meshes
class Model : public Renderable {
protected:
    Scene* m_scene;
    
    std::vector<Mesh*> m_modelMeshes;
    glm::vec3 m_position;
    glm::vec3 m_facing;
    
    void generateMeshesRecursively(ShaderProgram* shader, Scene* scene, aiNode* node, const aiScene* aiscene, std::string texturefolder);
    
public:
    Model(ShaderProgram* shader, Scene* scene, std::string objFilePath);
    ~Model();
    
    void render(Mode mode) final;
    void renderToShadowMap() final;
    void renderBoundingBox(Mode m);
    
    bool collision(Model* m);
    
    glm::vec3 position() { return m_position; };
    glm::vec3 facing() { return m_facing; };
    
    void setPosition(glm::vec3 p);
    void setSize(float s);
};

/***********************************************************
                    Derived Classes
 ***********************************************************/

class Character : public Model {
    std::clock_t m_timerStart;
    
    const float m_movementSpeed = 0.7f;
    const float m_glideDuration = 0.4f;
    
    float getDepthDampeningFactor();
    
public:
    Character(ShaderProgram* shader, Scene* scene);
    
    void reset();
    void glide();
    
    void turnLeft();
    void turnRight();
    void forward();
};

// ------------------------------

class Fish : public Model {
    int m_id;
    
    std::mt19937 m_generator;
    std::uniform_real_distribution<float> m_dist;
    float m_rand() { return m_dist(m_generator); };  // Returns a random double in (0, 10]
    
    float m_movementSpeed;
    std::clock_t m_timerStart;
    
    void rotateYAxis(float angle);
    bool collisionExists();
    
public:
    Fish(ShaderProgram* shader, Scene* scene, int id);
    
    void reset();
    void swim();
    
    int id() { return m_id; };
};

// ------------------------------

class TerrainObject : public Model {
    
public:
    TerrainObject(ShaderProgram* shader, Scene* scene, std::string path);
    
    void setOnTerrain(float x, float z);
};
