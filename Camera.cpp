#include "Camera.hpp"

using namespace std;
using namespace glm;

Camera::Camera(float w, float h) : m_angleToPlayer(0), m_pitch(25.0f), m_zoom(70), m_player(nullptr),
    m_near(0.1f), m_far(11000.0f), // Note that really far clipping plane is necessary for the sun
    m_facing(0.0f), m_thirdPersonView(true)
{
    m_proj = perspective(radians(45.0f),
                         w / h,   // aspect
                         m_near,
                         m_far);
};

void Camera::reset()
{
    if (m_thirdPersonView)
    {
        m_angleToPlayer = 0.0f;
        m_pitch = 25.0f;
        m_zoom = 70.0f;
    }
    else
    {
        m_angleToPlayer = 0.0f;
        m_pitch = 0.0f;
    }
    calculatePosition();
}

void Camera::setCharacter(Character* p)
{
    m_player = p;
    calculatePosition();    // initializes start position
};

void Camera::changePitch(float degrees)
{
    if (m_thirdPersonView)
    {
        // Clamp angle to the [15, 89] range
        m_pitch = clamp(m_pitch + degrees, 0.0f, 89.0f);
    }
    else
    {
        // Clamp angle to the [-30, 89] range
        m_pitch = clamp(m_pitch + degrees, -30.0f, 89.0f);
    }
    
    calculatePosition();
}


void Camera::invertAroundWater()
{
    m_pitch = -m_pitch;
    
    calculatePosition();
    
    if (!m_thirdPersonView)
    {
        // At this point, we have to move the camera down by 2 X the height at which
        // it floats above the water's surface
        m_position.y -= 10.0f;
    }
}

// Undoes the above operation
void Camera::revertAroundWater()
{
    m_pitch = -m_pitch;
    
    calculatePosition();
}

void Camera::rotateAroundPlayer(float degrees)
{
    m_angleToPlayer += degrees;
    
    // Clamp angle to the [0, 360] range
    if (m_angleToPlayer > 360.0f) m_angleToPlayer -= 360.0f;
    
    calculatePosition();
}

void Camera::characterMoved()
{
    calculatePosition();
}

void Camera::zoom(float dz)
{
    if (!m_thirdPersonView) return;  // no-op for 1st person view
    
    // Clamp zoom level to the [15, 100] range
    m_zoom = clamp(m_zoom - dz, 15.0f, 100.0f);
    
    calculatePosition();
}

mat4 Camera::projMatrix()
{
    return m_proj;
};

vec3 Camera::position()
{
    return m_position;
};

mat4 Camera::viewMatrix()
{
    vec3 target;
    if (m_thirdPersonView)
    {
        // In order for "invert pitch" to work correctly (aka invert about the Y = 0 plane, not Y = player height), we'll
        // make the camera track only the player's X & Z position
        target = vec3(m_player->position().x, 0, m_player->position().z);
    }
    else
    {
        target = m_position + m_facing;
    }
    
    return lookAt(m_position,
                  target,
                  glm::vec3(0.0f, 1.0f, 0.0f)); // up is always y axis bc we're not allowing any roll
};

float Camera::near()
{
    return m_near;
};

float Camera::far()
{
    return m_far;
};

void Camera::calculatePosition()
{
    if (m_thirdPersonView)
    {
        // We'll calculate the vector from the player's location to the camera using m_pitch & m_angleToPlayer,
        // then add this to the player location to get the camera location
        
        float distanceFromPlayer = m_zoom;
        
        // We'll break the player -> camera vector into 2 components: 1 in the X axis, 1 in the Y axis
        vec3 yComponentDirection = vec3(0, 1, 0);
        float yComponentLength = distanceFromPlayer * sin(radians(m_pitch));
        
        vec3 xComponentDirection = normalize(rotateY(m_player->facing(), radians(m_angleToPlayer)));
        float xComponentLength = distanceFromPlayer * cos(radians(m_pitch));
        
        // Same as in viewMatrix - we'll remove any Y component to the player's position
        vec3 target = vec3(m_player->position().x, 0, m_player->position().z);
        
        m_position = target +
        (yComponentLength * yComponentDirection) +
        (xComponentLength * xComponentDirection);
    }
    else
    {
        // Position is simply right above the player's position
        m_position = vec3(m_player->position().x, 5.0f, m_player->position().z);
        
        // Calculate the direction we are facing using m_pitch & m_angleToPlayer
        m_facing =  normalize(rotateY(-m_player->facing(), radians(m_angleToPlayer))) // X & Z components
                    + vec3(0.0f, sin(radians(m_pitch)), 0.0f);   // Y component
    }
}

void Camera::setThirdPersonView(bool b)
{
    if (m_thirdPersonView == b) return; // nothing to be done
    m_thirdPersonView = b;
    
    reset();
}
