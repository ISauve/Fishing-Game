#include "FrameBuffer.hpp"

FrameBuffer::FrameBuffer(int w, int h, bool drawcolor) : m_texture(0), m_depthTexture(0), m_depthRenderBuffer(0), m_width(w), m_height(h)
{
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    if (drawcolor)  glDrawBuffer(GL_COLOR_ATTACHMENT0);
    else            glDrawBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    CHECK_GL_ERRORS;
}

FrameBuffer::~FrameBuffer()
{
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_texture);
    glDeleteTextures(1, &m_depthTexture);
    glDeleteRenderbuffers(1, &m_depthRenderBuffer);
    
    CHECK_GL_ERRORS;
}

void FrameBuffer::bind()
{
    glBindTexture(GL_TEXTURE_2D, 0); // ensure texture isn't bound
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // Could change resolution of buffer here
    // Not doing this because reflection/refraction modes look best at full resolution
    // glViewport(0, 0, m_width, m_height);
}

void FrameBuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // glViewport(0, 0, display m_width, display m_height);
}

void FrameBuffer::addTextureAttachment()
{
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
    
    CHECK_GL_ERRORS;
}

void FrameBuffer::addDepthTextureAttachment()
{
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    
    CHECK_GL_ERRORS;
}

void FrameBuffer::addDepthRenderBuffer()
{
    glGenRenderbuffers(1, &m_depthRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);
    
    CHECK_GL_ERRORS;
}

GLuint FrameBuffer::texture()
{
    return m_texture;
}

GLuint FrameBuffer::depthTexture()
{
    return m_depthTexture;
}
