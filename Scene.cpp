#include "Scene.hpp"

using namespace std;
using namespace glm;

Scene::Scene(Camera* c, int w, int h, ShaderProgram* shadowShader) :
    m_renderBoundingBoxes(false), m_camera(c), m_shadowShader(shadowShader),
    m_reflection(w, h, true), m_refraction(w, h, true), m_shadowMap(w, h, false)
{
    // Initialize the extra framebuffers which we will render to
    m_reflection.bind();
        m_reflection.addTextureAttachment();
        m_reflection.addDepthRenderBuffer();
    m_reflection.unbind();
    
    m_refraction.bind();
        m_refraction.addTextureAttachment();
        m_refraction.addDepthTextureAttachment();
    m_refraction.unbind();
    
    m_shadowMap.bind();
        m_shadowMap.addDepthTextureAttachment();
    m_shadowMap.unbind();
};

Scene::~Scene()
{
    delete m_camera;
    delete m_sun;
    delete m_lensflare;
    delete m_skybox;
    delete m_water;
    for (auto renderable : m_renderables) delete renderable;
    for (auto img : m_images) delete img;
};

// Setters ---------------------------------------------------------------------------------

void Scene::setSun(Sun* s)
{
    m_sun = s;
    // don't add to m_renderables because this needs to be rendered separately
};

void Scene::setLensFlare(LensFlare *l)
{
    m_lensflare = l;
    // don't add to m_renderables because this needs to be rendered separately
};

void Scene::setSkybox(Skybox* s)
{
    m_skybox = s;
    // don't add to m_renderables because this needs to be rendered separately
};

void Scene::setWater(Water* w)
{
    m_water = w;
    // don't add to m_renderables because this needs to be rendered separately
};

void Scene::setTerrain(Terrain* t)
{
    m_terrain = t;
    addRenderable(t);
};

void Scene::setCharacter(Character* c)
{
    m_character = c;
    addRenderable(c);
    
    // The second a character is made, it has to be added to the camera
    m_camera->setCharacter(c);
};

void Scene::addFish(Fish* f)
{
    m_fish.push_back(f);
    addRenderable(f);
};

void Scene::setCurrScore(Image2D* s)
{
    m_currScore = s;
    addImage(s);
};

void Scene::addImage(Image2D* i)
{
    // don't add to m_renderables because this needs to be rendered separately
    m_images.push_back(i);
};

void Scene::addTerrainObject(TerrainObject* t)
{
    addRenderable(t);
}

void Scene::removeFish(int id)
{
    for (int i=0; i<m_fish.size(); i++)
    {
        if (m_fish[i]->id() == id) {
            // Note that we keep the caught fish in a separate vector so that they aren't deleted
            // until the end, that way their textures aren't released the game is over
            m_caughtFish.push_back(m_fish[i]);
            m_fish.erase(m_fish.begin() + i);
            return;
        }
    }
}

void Scene::reset()
{
    // Reset the camera view, the player's position & the fish
    m_camera->setThirdPersonView(true);
    m_camera->reset();
    m_character->reset();
    
    m_fish.insert(m_fish.end(), m_caughtFish.begin(), m_caughtFish.end());
    m_caughtFish.clear();
    
    for (auto fish : m_fish)
        fish->reset();
    
    m_currScore->setImage("Numbers/0.png");
}

// Rendering ---------------------------------------------------------------------------------

void Scene::render()
{
    /* 1) Render the reflection, refraction, and shadow map textures to their respective framebuffers */
    m_camera->calculatePosition();  // make sure our camera's position is up to date
    render(REFRACTION, &m_refraction);
    
    // Before rendering the reflection texture, we need to flip the camera in the Y axis about the water level
    m_camera->invertAroundWater();
    render(REFLECTION, &m_reflection);
    m_camera->revertAroundWater();    // reset the camera
   
    generateShadowMap();
    
    /* 2) Render result to output buffer */
    render(REGULAR, nullptr);
    
    glFlush();  // unsure if this is necessary - keeping it to be safe
};

void Scene::render(Mode mode, FrameBuffer* framebuffer)
{
    if (framebuffer) framebuffer->bind();
 
    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);   // accept fragment if closer to camera
    
    // Enable clipping
    glEnable(GL_CLIP_DISTANCE0);
    
    // Enable blending to create transparency effect if a < 1
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear the screen
    glClearColor(0.529, 0.808, 0.922, 1.0);   // sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Turn off the depth mask for rendering the skybox & sun so that they always gets overwritten
    glDepthMask(GL_FALSE);
    m_skybox->render(REGULAR);
    if (m_skybox->isDay()) m_sun->render(REGULAR);
    glDepthMask(GL_TRUE);
    
    for (auto renderable : m_renderables) {
        // don't render caught fish
        if (find(m_caughtFish.begin(), m_caughtFish.end(), renderable) != m_caughtFish.end()) continue;
        
        renderable->render(mode);
    }
    
    // Don't render water in reflection/refraction textures
    // Note: water rendering mode is modified in FishingGame.cpp via ImGui inputs
    if (mode == REGULAR) {
        m_water->render(REGULAR);
        
        // Render the 2D images after we render the water, that way any alpha blending in the image
        // will properly blend with the water
//        for (auto img : m_images) img->render(REGULAR);
        
        // Same with the lens flare
        if (m_skybox->isDay()) {
            glDepthMask(GL_FALSE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Lens flare uses additive blending
            m_lensflare->render(REGULAR);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    
    if (m_renderBoundingBoxes) {
        for (auto fish : m_fish)
            fish->renderBoundingBox(mode);
        m_character->renderBoundingBox(mode);
    }
    
    if (framebuffer) framebuffer->unbind();
    CHECK_GL_ERRORS;
}

void Scene::generateShadowMap()
{
    m_shadowMap.bind();
    glEnable(GL_DEPTH_TEST);
    glClear( GL_DEPTH_BUFFER_BIT);
    
    // Generate the view & projection matrices
    mat4 proj = m_sun->orthographicProjMatrix();
    mat4 view = m_sun->viewMatrix();
    
    // Upload those matrices to the shader map shader
    m_shadowShader->enable();
    
    GLint location = m_shadowShader->getUniformLocation("Projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(proj));

    location = m_shadowShader->getUniformLocation("View");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(view));
    
    // Render the objects
    for (auto renderable : m_renderables) {
        // Don't render caught fish
        if (find(m_caughtFish.begin(), m_caughtFish.end(), renderable) != m_caughtFish.end()) continue;
        
        renderable->renderToShadowMap();
    }
    
    m_shadowShader->disable();
    m_shadowMap.unbind();
}
