#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include "cs488-framework/ShaderProgram.hpp"
#include "cs488-framework/GlErrorCheck.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>

class FrameBuffer {
    GLuint m_fbo;
    GLuint m_texture;
    GLuint m_depthTexture;
    GLuint m_depthRenderBuffer;
    int m_width;
    int m_height;
    
public:
    FrameBuffer(int w, int h, bool drawcolor);
    ~FrameBuffer();
    
    void bind();
    void unbind();
    
    void addTextureAttachment();
    void addDepthTextureAttachment();
    void addDepthRenderBuffer();
    
    // Accessors
    GLuint texture();
    GLuint depthTexture();
};
