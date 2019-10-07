#include "FishingGame.hpp"

#include "cs488-framework/GlErrorCheck.hpp"
#include "Renderable.hpp"
#include "LensFlare.hpp"
#include "Scene.hpp"
#include <imgui/imgui.h>
#include <iostream>

using namespace std;
using namespace glm;

FishingGame::FishingGame() :
    m_mouseDown(false),
    m_showSettings(false),
    m_thirdPersonView(true),
    m_renderBoundingBoxes(false),
    m_waterDistortion(0.63f),
    m_bumpMapping(true),
    m_skyboxRotationSpeed(0.1f),
    m_currMode(Mode::REGULAR),
    m_currScore(0)
{}

FishingGame::~FishingGame()
{
    delete m_scene;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void FishingGame::init()
{
    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);
    
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    m_lastMousePos = vec2(xpos, ypos);

    // Initialize the scene
    m_scene = new Scene(new Camera(m_framebufferWidth, m_framebufferHeight),
                        m_framebufferWidth, m_framebufferHeight,
                        generateShader("ShadowMapVtxShader.vs", "ShadowMapFragShader.fs"));
    
    // Create the shaders
    ShaderProgram* image2DShader = generateShader("2DImageVtxShader.vs", "2DImageFragShader.fs");
    ShaderProgram* skyboxShader = generateShader("SkyboxVtxShader.vs", "SkyboxFragShader.fs");
    ShaderProgram* waterShader = generateShader("WaterVtxShader.vs", "WaterFragShader.fs");
    ShaderProgram* objectShader = generateShader("ObjectVertexShader.vs", "ObjectFragmentShader.fs");
    
    // Create the renderables and add them to the scene
    Sun* s = new Sun(image2DShader, m_scene);
        s->setSize(2000.0f);
        m_scene->setSun(s);
    
    LensFlare* l = new LensFlare(image2DShader, m_scene);
        m_scene->setLensFlare(l);
    
    Skybox* sk = new Skybox(skyboxShader, m_scene, m_skyboxRotationSpeed);
        m_scene->setSkybox(sk);
    
    Water* w = new Water(waterShader, m_scene);
        w->setSize(500);
        m_scene->setWater(w);

    Terrain* t = new Terrain(objectShader, m_scene, 2000, 100);
        t->setPosition(vec3(-1 * t->getSize() / 2.0f,
                            5.2,
                            -1 * t->getSize() / 2.0f));
        m_scene->setTerrain(t);
    
    cout << "Loading boat model..." << endl;
    Character* c = new Character(objectShader, m_scene);
        c->setPosition(vec3(0, -1, 0));
        c->setSize(1.3);
        m_scene->setCharacter(c);

    cout << "Loading fish models..." << endl;
    for (int i=0; i<10; i++) {
        Fish* f = new Fish(objectShader, m_scene, i);
        f->setSize(0.3);
        m_scene->addFish(f);
    }
    
    // For the images below, note that the position is in NDC and and size is relative to the size of the screen
    Image2D* currScore = new Image2D(image2DShader, m_scene, "Numbers/0.png", vec2(0.065f, 0.065f));
        currScore->setPosition(vec3(0.75f, 0.9f, 0.0f));
    
    Image2D* slash = new Image2D(image2DShader, m_scene, "Numbers/Slash.png", vec2(0.05f, 0.05f));
        slash->setPosition(vec3(0.825f, 0.8f, 0.0f));
    
    Image2D* outOf = new Image2D(image2DShader, m_scene, "Numbers/10.png", vec2(0.065f, 0.065f));
        outOf->setPosition(vec3(0.9f, 0.7f, 0.0f));

    m_scene->setCurrScore(currScore);
    m_scene->addImage(outOf);
    m_scene->addImage(slash); // render this last so the other 2 get rendered on top
    
    // Add some terrain objects to decorate the terrain
    struct TerrainObjectData {
        float xPosition;
        float zPosition;
        float size;
        TerrainObjectData(float x, float z, float s) : xPosition(x), zPosition(z), size(s) {};
    };
    
    cout << "Loading rock models..." << endl;
    vector<TerrainObjectData> rocks {
        TerrainObjectData(70.0f, 0.0f, 2.0f),
        TerrainObjectData(-175.0f, -135.0f, 3.7f),
    };
    for (auto data : rocks) {
        TerrainObject* rock = new TerrainObject(objectShader, m_scene, "Assets/Rock/Rock.obj");
        rock->setOnTerrain(data.xPosition, data.zPosition);
        rock->setSize(data.size);
        m_scene->addTerrainObject(rock);
    }
    
    cout << "Loading tree models..." << endl;
    vector<TerrainObjectData> trees {
        TerrainObjectData(-150.0f, -150.0f, 2.7f + 10.0f),
        TerrainObjectData(-300.0f, 100.0f, 3.0f + 10.0f),
        TerrainObjectData(-90.0f, 200.0f, 2.5f + 10.0f),
    };
    for (auto data : trees) {
//        TerrainObject* tree = new TerrainObject(objectShader, m_scene, "Assets/Tree/Tree.obj");
        TerrainObject* tree = new TerrainObject(objectShader, m_scene, "Assets/LowPolyTree/lowpolytree.obj"); // speed increase
        tree->setOnTerrain(data.xPosition, data.zPosition);
        tree->setSize(data.size);
        m_scene->addTerrainObject(tree);
    }
    
    cout << "Ready to play!" << endl;
}

// Helper function which generates a shader program & stores it
ShaderProgram* FishingGame::generateShader(string vtxShader, string fragShader)
{
    ShaderProgram* shader = new ShaderProgram();
    shader->generateProgramObject();
    shader->attachVertexShader( ("Assets/Shaders/" + vtxShader).c_str() );
    shader->attachFragmentShader( ("Assets/Shaders/" + fragShader).c_str() );
    shader->link();
    
    m_shaders.push_back(shader);
    
    return shader;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void FishingGame::appLogic()
{
    // Poll for events
    glfwPollEvents();
    handleRepeatInput();
    
    m_scene->character()->glide();
 
    // Update fish positions
    for (auto fish : m_scene->fish())
        fish->swim();
    
    // Check if we've caught any fish
    for (auto fish : m_scene->fish()) {
        if (fish->collision(m_scene->character())) {
            m_scene->removeFish(fish->id());
            m_currScore++;
            m_scene->currScore()->setImage("Numbers/" + to_string(m_currScore) + ".png");
        }
    }
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void FishingGame::guiLogic()
{
    static bool firstRun(true);
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(50, 50));
        firstRun = false;
    }
    
    if (m_showSettings) {
        static bool showDebugWindow(true);
        ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
        float opacity(0.5f);
        
        ImGui::Begin("Information", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
    
            ImGui::Text("Move character with W/A/S");
            ImGui::Text("Press I to hide information");
        
            ImGui::Checkbox("Render bounding boxes", &m_renderBoundingBoxes);
            m_scene->renderBoundingBoxes(m_renderBoundingBoxes);
        
            ImGui::Checkbox("Third person view", &m_thirdPersonView);
            m_scene->camera()->setThirdPersonView(m_thirdPersonView);
        
            ImGui::Checkbox("Bump Mapping", &m_bumpMapping);
            m_scene->water()->setBumpMapping(m_bumpMapping);
        
            ImGui::SliderFloat("Water Distortion", &m_waterDistortion, 0.0f, 1.0f, "%.2f");
            m_scene->water()->setDistortion(m_waterDistortion);
        
            ImGui::SliderFloat("Speed of Time", &m_skyboxRotationSpeed, 0.0f, 10.0f, "%.2f");
            m_scene->skybox()->setRotationSpeed(m_skyboxRotationSpeed);
        
            ImGui::Text("Water rendering mode:");
            ImGui::PushID( 0 );
            if( ImGui::RadioButton( "Regular         ", &m_currMode, REGULAR) ) {}
            if( ImGui::RadioButton( "Reflection only ", &m_currMode, REFLECTION) ) {}
            if( ImGui::RadioButton( "Refraction only ", &m_currMode, REFRACTION) ) {}
            if( ImGui::RadioButton( "Bump map        ", &m_currMode, BUMP_MAP) ) {}
            ImGui::PopID();
            m_scene->water()->setMode((Mode) m_currMode);
        
            ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );
        
            if ( ImGui::Button( "Reset              (R)" ) ) {
                m_scene->reset();
                m_currScore = 0;
            }
        
        
            if ( ImGui::Button( "Quit Application   (Q)" ) )
                glfwSetWindowShouldClose(m_window, GL_TRUE);
        
        ImGui::End();
    }
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void FishingGame::draw()
{
    m_scene->render();
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void FishingGame::cleanup()
{
    for (ShaderProgram* shader : m_shaders)
        delete shader;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool FishingGame::mouseMoveEvent (double xPos, double yPos)
{
	bool eventHandled(false);
    
    if (m_mouseDown) {
        // Adjust pitch based on change in y
        float dy = m_lastMousePos.y - yPos;
        m_scene->camera()->changePitch(dy * 0.3);
        
        // Adjust angle around the player based on change in x
        float dx = m_lastMousePos.x - xPos;
        m_scene->camera()->rotateAroundPlayer(dx * 0.3);
    }
    
    m_lastMousePos = vec2(xPos, yPos);
    eventHandled = true;

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool FishingGame::mouseButtonInputEvent (int button, int actions, int mods)
{
	bool eventHandled(false);
    
    if ( !ImGui::IsMouseHoveringAnyWindow() ) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            m_mouseDown = actions == GLFW_PRESS;
            eventHandled = true;
        }
    }

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool FishingGame::keyInputEvent (int key, int action, int mods)
{
    bool eventHandled(false);
    
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(m_window, GL_TRUE);
                eventHandled = true;
                break;
                
            case GLFW_KEY_I:
                m_showSettings = !m_showSettings;
                eventHandled = true;
                break;
                
            case GLFW_KEY_R:
                m_scene->reset();
                m_currScore = 0;
                eventHandled = true;
                break;
                
            default: break;
        }
    }
    
    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll events.
 */

bool FishingGame::mouseScrollEvent(double xOffSet, double yOffSet) {
    bool eventHandled(true);
    m_scene->camera()->zoom(yOffSet);
    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles repeat key input events.
 */

void FishingGame::handleRepeatInput() {
    if (glfwGetKey(m_window, GLFW_KEY_A)) m_scene->character()->turnLeft();
    if (glfwGetKey(m_window, GLFW_KEY_D)) m_scene->character()->turnRight();
    if (glfwGetKey(m_window, GLFW_KEY_W)) m_scene->character()->forward();
}

