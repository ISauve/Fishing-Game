#include "Model.hpp"
#include "Scene.hpp"
#include <string>

using namespace std;
using namespace glm;

Character::Character(ShaderProgram* shader, Scene* scene) : Model(shader, scene, "Assets/Boat/boat.obj")
{
    
}

void Character::reset()
{
    m_position = vec3(0.0);
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(vec3(0.0));
    
    m_facing = vec3(0.0f, 0.0f, -1.0f);
    for (Mesh* mesh : m_modelMeshes)
        mesh->setRotation(vec3(0.0));
}

float Character::getDepthDampeningFactor()
{
    // Dampen movement by depth of water below the boat
    float shallowestPoint = -numeric_limits<float>::infinity();
    for (Mesh* mesh : m_modelMeshes)
    {
        vec3 frontOfMesh = vec3(mesh->modelMatrix() * vec4(mesh->maxBounds(), 1.0f));
        float depth = m_scene->terrain()->getHeightAt(frontOfMesh.x, frontOfMesh.z);
        shallowestPoint = (depth > shallowestPoint) ? depth : shallowestPoint;
        
        vec3 backOfMesh = vec3(mesh->modelMatrix() * vec4(mesh->minBounds(), 1.0f));
        depth = m_scene->terrain()->getHeightAt(backOfMesh.x, backOfMesh.z);
        shallowestPoint = (depth > shallowestPoint) ? depth : shallowestPoint;
    }
    
    // Dampen movement by depth of water at the front of the boat
    float dampeningFactor = 1.0f;
    if (shallowestPoint >= -3.0f)     // Stop at -3
        dampeningFactor = 0.0f;
    else if (shallowestPoint > -7.0f) // Start slowing down at -7
        dampeningFactor *= 1.0f - ((shallowestPoint + 7.0f) / 4.0f);
    
    return dampeningFactor;
}

void Character::glide()
{
    // Continue to move the player forward at a pace relative to how long ago they last moved
    float timeSinceStart = (clock() - m_timerStart) / (float) CLOCKS_PER_SEC;
    if (timeSinceStart > m_glideDuration) return;  // movement has stopped
    
    float movement = (m_glideDuration - timeSinceStart) * (1.0f / m_glideDuration);
    movement *= getDepthDampeningFactor();
    
    m_position = m_position - movement * m_movementSpeed * m_facing;
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(mesh->position() - movement * m_movementSpeed * m_facing);
}

void Character::turnLeft()
{
    // Rotate 2 degrees in the Y axis
    mat4 rotation = rotate(mat4(1.0f), radians(2.0f), vec3(0.0f, 1.0f, 0.0f));
    m_facing = normalize(vec3(rotation * vec4(m_facing, 1.0f)));
    for (Mesh* mesh : m_modelMeshes)
        mesh->rotate(vec3(0, 2, 0));
};

void Character::turnRight()
{
    // Rotate -2 degrees in the Y axis
    mat4 rotation = rotate(mat4(1.0f), radians(-2.0f), vec3(0.0f, 1.0f, 0.0f));
    m_facing = normalize(vec3(rotation * vec4(m_facing, 1.0f)));
    for (Mesh* mesh : m_modelMeshes)
        mesh->rotate(vec3(0, -2, 0));
};

void Character::forward()
{
    m_timerStart = clock();    // start the movement timer
    
    float dampeningFactor = getDepthDampeningFactor();
    m_position = m_position - dampeningFactor * m_movementSpeed * m_facing;
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(mesh->position() - dampeningFactor * m_movementSpeed * m_facing);
};
