#include "LensFlare.hpp"
#include "Scene.hpp"

using namespace std;
using namespace glm;

/*
 The textures used below and technique for determining the placement of those textures
 comes from https://www.youtube.com/watch?v=OiMRdkhvwqg&list=PLRIWtICgwaX0u7Rf9zkZhLoLuZVfUksDP&index=57
 */

LensFlare::LensFlare(ShaderProgram* shader, Scene* scene) : Renderable(), m_scene(scene)
{
    // Initialize the images
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex6.png", vec2(0.5f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex4.png", vec2(0.023f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex2.png", vec2(0.1f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex7.png", vec2(0.05f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex3.png", vec2(0.06f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex5.png", vec2(0.07f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex7.png", vec2(0.2f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex3.png", vec2(0.6f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex5.png", vec2(0.3f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex4.png", vec2(0.4f)));
    m_images.push_back(new Image2D(shader, m_scene, "LensFlare/tex8.png", vec2(0.6f)));
}

LensFlare::~LensFlare()
{
    for (auto img : m_images) delete img;
}

void LensFlare::render(Mode mode)
{
    if (m_scene->camera()->isThirdPerson()) return; // don't render in 3rd person mode - this just causes bugs
    
    /* Step 1) Get the position of the sun in screen coordinates */
    vec4 sunPosition = m_scene->camera()->projMatrix() * m_scene->camera()->viewMatrix() * vec4(m_scene->sun()->position(), 1.0f);
    vec2 sunScreenCoords = vec2(sunPosition.x / sunPosition.w, sunPosition.y / sunPosition.w); // Perspective division
    if (sunScreenCoords.x > 1.0f || sunScreenCoords.x < -1.0f ||
        sunScreenCoords.y > 1.0f || sunScreenCoords.y < -1.0f)
        return; // The sun is not currently on the screen, so we don't need to render anything
    
    /* Step 2) Calculate vector from sun position through the center of the screen & use this to determine brightness of effect */
    vec2 sunToCenterOfScreen = vec2(0.0f, 0.0f) - sunScreenCoords;
    float brightness = 1.0f - length(sunToCenterOfScreen) / 0.5f;
    if (brightness <= 0.0f)
        return; // The sun isn't centered enough for us the lens flare effect to be present
    
    // Need to prevent flare from being rendered when the sun is blocked from view by the terrain
    // We know the sun drops behind the horizon at time 0.8754 & rises at time 0.122056
    float timeUntilSunrise = 0.122056f - m_scene->skybox()->time();
    if (timeUntilSunrise > 0.05f)
        return;
    else if (timeUntilSunrise >= 0.0f)
        brightness *= 1.0f - (timeUntilSunrise * 20.0f); // fade from a brightness of 0 to 1 as the time to sunrise decreases
    
    float timeSinceSundown = m_scene->skybox()->time() - 0.8754f;
    if (timeSinceSundown > 0.05f)
        return;
    else if (timeSinceSundown >= 0.0f)
        brightness *= 1.0f - (timeSinceSundown * 20.0f); // fade from a brightness of 1 to 0 as the time since sundown increases
    
    /* Step 3) Calculate the positions of each flare texture */
    float spacing = 0.4;
    for (int i=0; i<m_images.size(); i++) {
        vec2 position = sunScreenCoords + i * spacing * sunToCenterOfScreen;
        m_images[i]->setPosition(vec3(position.x, position.y, 0.0));
        m_images[i]->setTransparency(brightness);
    }

    // Render them
    for (auto img : m_images) img->render(mode);
}

