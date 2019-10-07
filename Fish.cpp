#include "Model.hpp"
#include "Scene.hpp"
#include <string>

using namespace std;
using namespace glm;

Fish::Fish(ShaderProgram* shader, Scene* scene, int id) : Model(shader, scene, "Assets/Fish/fish.obj"), m_id(id)
{
    // Initialize our random number generator
    auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
    m_generator = mt19937(seed);
    m_dist = uniform_real_distribution<float>(0,10);
    
    reset();
}

void Fish::reset()
{
    while (true) {
        // Randomly generate a starting position
        m_position = vec3( (80.0f - (m_rand() * 8.0f)) - 150.0f, // Ranges from [-80, -150]
                          -3.0f,
                          m_rand() * 7.0f + 50.0f); // Ranges from [50, 120]
        
        for (Mesh* mesh : m_modelMeshes) mesh->setPosition(m_position);
        
        // Randomly generate a starting direction
        float rotationAngle = m_rand() * 36.0f; // Ranges from [0, 360]
        rotateYAxis(rotationAngle);
        
        // Check for a collision - we don't want to initialize our fish on top of each other
        if (collisionExists()) continue; // Try again!
        
        break;
    }
    
    // Randomly generate a movement speed
    m_movementSpeed = m_rand() / 40.0f + 0.2f; // Ranges from [0.25, 0.5]
}

void Fish::rotateYAxis(float rotationAngle)
{
    mat4 rotation = rotate(mat4(1.0f), radians(rotationAngle), vec3(0.0f, 1.0f, 0.0f));
    m_facing = normalize(vec3(rotation * vec4(m_facing, 1.0f)));
    for (Mesh* mesh : m_modelMeshes)
        mesh->rotate(vec3(0, rotationAngle, 0));
}

void Fish::swim()
{
    // Periodically generate a small change in direction
    if (m_rand() < 0.2f) {  // 2% of the time
        float rotationAngle = m_rand() * 6.0f - 30.0f; // Ranges from [-30, 30]
        rotateYAxis(rotationAngle);
    }
    
    // Move the fish forward in the direction it's facing
    m_position -= m_movementSpeed * m_facing;
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(mesh->position() - m_movementSpeed * m_facing);

    if (collisionExists()) {
        // Turn in the opposite direction
        rotateYAxis(180.0f);
        
        // Move back in that direction
        m_position -= m_movementSpeed * m_facing;
        for (Mesh* mesh : m_modelMeshes)
            mesh->setPosition(mesh->position() - m_movementSpeed * m_facing);
    }
}

bool Fish::collisionExists()
{
    // Check for collisions with other fish
    for ( Fish* fish : m_scene->fish() ) {
        if (fish->id() == m_id) continue;
        
        if ( fish->collision(this) ) return true;
    }
    
    // To check for terrain collision, just check distance to the ground & make sure it's deep enough
    if (m_scene->terrain()->getHeightAt(m_position.x, m_position.z) > -3.5f) return true;
    
    return false;
}
