#pragma once

#include "Camera.hpp"
#include "Renderable.hpp"
#include "Object.hpp"
#include "Model.hpp"
#include "LensFlare.hpp"
#include "FrameBuffer.hpp"
#include "Mode.hpp"

// Container class which holds all of our objects
class Scene {
    bool m_renderBoundingBoxes;
    
    Camera*        m_camera;
    ShaderProgram* m_shadowShader;
    
    // Objects that will be rendered
    std::vector<Renderable*> m_renderables;
    
    // Pointers to important renderables that will need to be accessed later
    Sun*                  m_sun;
    LensFlare*            m_lensflare;
    Skybox*               m_skybox;
    Water*                m_water;
    Terrain*              m_terrain;
    Character*            m_character;
    std::vector<Fish*>    m_fish;
    std::vector<Fish*>    m_caughtFish;
    Image2D*              m_currScore;
    std::vector<Image2D*> m_images;

    // Framebuffers
    FrameBuffer m_reflection;
    FrameBuffer m_refraction;
    FrameBuffer m_shadowMap;
    
    // Helpers
    void addRenderable(Renderable* r) { m_renderables.push_back(r); };
    void render(Mode m, FrameBuffer* framebuffer);
    void generateShadowMap();
    
public:
    Scene(Camera* c, int framebufferW, int framebufferH, ShaderProgram* shadowShader);
    ~Scene();
    
    void reset();
    void render();
    
    // Setters
    void setSun(Sun* s);
    void setLensFlare(LensFlare* l);
    void setSkybox(Skybox* s);
    void setWater(Water* w);
    void setTerrain(Terrain* t);
    void setCharacter(Character* c);
    void addFish(Fish* f);
    void setCurrScore(Image2D* s);
    void addImage(Image2D* i); // for other images which won't get changed later
    void addTerrainObject(TerrainObject* t);
    
    void renderBoundingBoxes(bool b) { m_renderBoundingBoxes = b; };
    
    // Modifiers
    void removeFish(int id);
    
    // Accessors
    Camera*             camera()      { return m_camera; };
    Sun*                sun()         { return m_sun; };
    Skybox*             skybox()      { return m_skybox; };
    Water*              water()       { return m_water; };
    Terrain*            terrain()     { return m_terrain; };
    Character*          character()   { return m_character; };
    std::vector<Fish*>  fish()        { return m_fish; };
    Image2D*            currScore()   { return m_currScore; };
    
    GLuint reflectionTexture()        { return m_reflection.texture(); };
    GLuint refractionTexture()        { return m_refraction.texture(); };
    GLuint refractionDepthTexture()   { return m_refraction.depthTexture(); };
    GLuint shadowMapTexture()         { return m_shadowMap.depthTexture(); };
    
    ShaderProgram* shadowShader() { return m_shadowShader; };
};
