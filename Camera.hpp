#pragma once

#include "Model.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

class Camera {
    Character* m_player;
    
    glm::mat4 m_proj;
    glm::vec3 m_position;
    
    // Clipping planes
    float m_near;
    float m_far;
    
    // Camera state
    bool m_thirdPersonView;
    GLfloat m_angleToPlayer;
    GLfloat m_pitch;
    GLfloat m_zoom;     // Only for third person view
    glm::vec3 m_facing; // Only for first person view
    
public:
    Camera(float w, float h);
    void setCharacter(Character* p);    // must be called ASAP after construction
    
    void reset();
    void setThirdPersonView(bool b);
    
    bool isThirdPerson() { return m_thirdPersonView; };
    
    /*
     * EVERYTHING FOLLOWING REQUIRES setCharacter BE CALLED FIRST
     */
    void changePitch(float degrees);
    void invertAroundWater();
    void revertAroundWater();
    void rotateAroundPlayer(float degrees);
    void characterMoved(); // triggers a recalculating of position so the camera follows the character
    void zoom(float dz);
    void calculatePosition();
    
    glm::mat4 projMatrix();
    glm::vec3 position();
    glm::mat4 viewMatrix();
    float near();
    float far();
};
