#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include "Mode.hpp"

// Common base class for the 3 type of renderable objects we have: Objects, Models & Lens Flare 
class Renderable {
public:
    Renderable() {};
    virtual ~Renderable() {};
    
    virtual void render(Mode m) = 0;
    virtual void renderToShadowMap() = 0;
};
