#pragma once

#define GL_SILENCE_DEPRECATION // silences warnings on macOS 10.14 related to deprecated OpenGL functions

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "Scene.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

class FishingGame : public CS488Window {
    Scene* m_scene;
    std::vector<ShaderProgram*> m_shaders;
    
    // Mouse state
    glm::vec2 m_lastMousePos;
    bool m_mouseDown;
    
    // Rendering state - modifiable by user input
    bool m_showSettings;
    bool m_thirdPersonView;
    bool m_renderBoundingBoxes;
    int m_currMode;
    float m_waterDistortion;
    bool m_bumpMapping;
    float m_skyboxRotationSpeed;
    
    // Game information
    int m_currScore;
    
    // Helpers
    ShaderProgram* generateShader(std::string vtxShader, std::string fragShader);
    void handleRepeatInput();
    
public:
	FishingGame();
	virtual ~FishingGame();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool mouseMoveEvent(double xPos, double yPos) override;
    virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;
};
