#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include "Renderable.hpp"
#include "Object.hpp"

// This class is a container of Image2D's
class LensFlare : public Renderable {
    Scene* m_scene;
    std::vector<Image2D*> m_images;
    
public:
    LensFlare(ShaderProgram* shader, Scene* scene);
    ~LensFlare();
    
    void render(Mode mode) final;
    void renderToShadowMap() final {}; // noop
};
