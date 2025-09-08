///////////////////////////////////////////////////////////////////////////////
///
///	@file Graphics.cpp
/// 
/// @brief Graphics system rendering pipeline. Includes all the functions for:
///        drawing of meshes, creating basic shapes, performing matrix transformation
///        and input of PNG images.
///	
///	@Authors: Victor Lee
///           Dylan Lau
///           Edwin Leow
///           Joel Teo
///           Joshua Sim
///           Keegan Lim
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <windows.h>
#include <commdlg.h>
#include "Graphics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize2.h"
#include <InputHandler.h>
#include "ComponentList.h"
#include "FontSystem.h"
#include "Coordinator.h"
#include "LogicManager.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "EngineState.h"
#include "AssetManager.h"
#include "TextureAsset.h"
#include "TagManager.h"
#include "Debugger.h"
#include "ParticleSystem.h"
#include "UndoSystem.h"

extern Framework::FontSystem fontSystem;
extern Framework::Coordinator ecsInterface;

namespace Framework {

    //initialising containers
    std::vector<Graphics::Model> Graphics::models{};
    std::unordered_map<std::string, Graphics::Model> Graphics::meshes{};
    std::unordered_map<std::string, GLuint> Graphics::textures{};
    std::vector<Entity> Graphics::sortedEntities{};
    // In your Graphics or main game class
    unsigned char* Graphics::data{};
    float Graphics::projWidth{};
    float Graphics::projHeight{};
    float Graphics::projMousex{};
    float Graphics::projMousey{};
    Graphics::Camera Graphics::camera({ 800, 600 });
    GLuint Graphics::gameFramebuffer{}, Graphics::gameTexture{}, Graphics::rbo{}, Graphics::pickingFBO{};
    bool print_out = false;

    static Entity selectedEntity = std::numeric_limits<Entity>::max();  // Sets a selected entity to be a non existent entity out of range
    static bool isPropertiesWindowOpen = false;                         // Allows opening and closing of the name editor? might want to remove

    static bool screenShake = false;
    // Shake parameters
    float shakeDuration = 0.0f;
    float shakeMagnitude = 10.0f; // Adjust as needed
    float shakeOffsetX = 0.0f;
    float shakeOffsetY = 0.0f;

    //Game viewport
    float Graphics::viewportWidth{};
    float Graphics::viewportHeight{};
    float Graphics::viewportOffsetX{};
    float Graphics::viewportOffsetY{};
    float Graphics::EmitX{};
    float Graphics::EmitY{};

    //** TESTING PURPOSES **//
    float sprite_x = 1150.f;
    int sprite_no = 5;
    float rotate_left = 0.0f;
    float rotate_right = 0.0f;
    float scale_left = 200.0f;
    float scale_right = 200.0f;


    // Loading of Graphics Shaders
    std::string const UE_vs = GlobalAssetManager.UE_LoadGraphicsShader("Assets/GraphicShaders/UE.vert");
    std::string const UE_vs2 = GlobalAssetManager.UE_LoadGraphicsShader("Assets/GraphicShaders/UE_Vertex.vert");
    std::string const UE_fs = GlobalAssetManager.UE_LoadGraphicsShader("Assets/GraphicShaders/UE.frag");
    std::string const UE_fs2 = GlobalAssetManager.UE_LoadGraphicsShader("Assets/GraphicShaders/UE_Tint.frag");

    // edwin
    auto& textureAssets = GlobalAssetManager.UE_GetAllTextureAssets();
    auto& audioAssets = GlobalAssetManager.UE_GetAllAudioAssets();
    auto& entityAssets = GlobalAssetManager.UE_GetAllEntities();
       
    static std::string selectedTextureName;
    static std::string selectedAudioName;
    static std::string filePath{};
    static std::string newTextureName = "";
    std::string newParticleTextureName;
    static std::string previousSelectedAudioName;
    static UndoRedoManager undoRedoManager;
    static bool hasAudioWin = false;
    static bool hasAudioLose = false;

    //imgui
    bool Graphics::toggleImGUI = true;

    // Error message and flag for popup
    std::string errorMessage;
    bool showErrorPopup = false;
    bool showGizmos = false; // Toggle to show/hide gizmos

    static bool modeChanged = false;  // Flag to track if the mode has changed
    static bool soundTypeChanged = false;
    static int selectedAudioPathIndex = 0;
    static char newAudioNameBuffer[256];   // Use a char buffer for ImGui::InputText
    std::unordered_map<std::string, bool> editingStates;
    std::vector<std::string> audioToDelete;

    std::vector<std::string> audioPathOptions
    {
        "Assets/Audio/bgm/",
        "Assets/Audio/soundeffect/"
    };

    // Declare the map for storing dropped texture files
    std::vector<std::pair<std::string, std::string>> nameUpdate;

    std::vector<glm::vec2> vertices =
    {
        glm::vec2(-0.5f, -0.5f), // Bottom-left
        glm::vec2(0.5f, -0.5f), // Bottom-right
        glm::vec2(0.5f,  0.5f), // Top-right
        glm::vec2(-0.5f,  0.5f)  // Top-left
    };

    std::vector<glm::vec2> texCoords =
    {
        glm::vec2(0.0f, 0.0f),  // Bottom-left
        glm::vec2(1.0f, 0.0f),  // Bottom-right
        glm::vec2(1.0f, 1.0f),  // Top-right
        glm::vec2(0.0f, 1.0f)   // Top-left
    };

    glm::vec3 color{ 1.0f, 1.0f, 1.0f };

    // Constructor
    Graphics::Graphics(GraphicsWindows* graphicWindows) : graphicWindows(graphicWindows)
    {
        InputHandlerInstance = InputHandler::GetInstance();
        projHeight = static_cast<float>(graphicWindows->getHeight());
        projWidth = static_cast<float>(graphicWindows->getWidth());
        projMousex = static_cast<float>(InputHandlerInstance->GetMouseX());
        projMousey = static_cast<float>(InputHandlerInstance->GetMouseY());
        camera.viewportSize = { projWidth, projHeight };
    }

    // Destructor
    Graphics::~Graphics()
    {
        // Clean up all models and unload textures
        for (int i = 0; i < models.size(); i++)
        {
            models[i].cleanup();
        }

        // Clear global maps after cleanup
        Graphics::models.clear();
        Graphics::textures.clear();
        Graphics::meshes.clear();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    // Initialize the system
    void Graphics::Initialize()
    {
        // Register graphics related components 
        ecsInterface.RegisterComponent<TransformComponent>();
        ecsInterface.RegisterComponent<RenderComponent>();
        ecsInterface.RegisterComponent<LayerComponent>();
        ecsInterface.RegisterComponent<TextComponent>();

        //  ---- ECS INITIALIZATION --- 
        Signature signature;
        signature.set(ecsInterface.GetComponentType<RenderComponent>()); //Set mEntities to be populated with RenderComponent holding Entities

        ecsInterface.SetSystemSignature<Graphics>(signature);
        std::cout << "Signature for Graphics system is: " << signature << std::endl;
        //  ---- ECS INITIALIZATION END --- 

        // --- Create Instances -----
        // ---- Create Input Handler Instance (Relocate to appropriate system *Graphic only renders*)
        InputHandlerInstance = InputHandler::GetInstance();

        //Initialize entry points to OpenGL functions and extensions
        if (!glfwInit())
        {
            std::cout << "GLFW init has failed - abort program!!!" << std::endl;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        glfwWindowHint(GLFW_RED_BITS, 8); glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8); glfwWindowHint(GLFW_ALPHA_BITS, 8);

        //Initialize entry points for glewInit()
        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            std::cerr << "Unable to initialize GLEW - error: "
                << glewGetErrorString(err) << " abort program" << std::endl;
        }

        //// Initial Setup - Should Run Once
        //IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking Support

        // Scale up all ImGui elements
        io.FontGlobalScale = 1.1f;

        // Additional docking configurations
        io.ConfigDockingAlwaysTabBar = true;
        io.ConfigDockingTransparentPayload = true;

        // Set ImGui Style
        ImGui::StyleColorsDark();
        gameFramebuffer = CreateFramebuffer(static_cast<int>(projWidth), static_cast<int>(projHeight), gameTexture, rbo);

        // Initialize backends
        ImGui_ImplGlfw_InitForOpenGL(graphicWindows->GetWindow(), true);
        ImGui_ImplOpenGL3_Init("#version 450");

        //INIT Font system
        fontSystem.Initialize();

        // IMPORTANT : setting color of background for program
        SetBackgroundColor(255, 255, 255, 255);

        // MAKE ONE MESH, USE GLOBALLY [1:1]
        createMesh(vertices, texCoords, color, "sprite", "bullet");
        createMesh(vertices, texCoords, color, "animation", "McIdleSprite");
    }

    // Update the system
    void Graphics::Update(float deltaTime)
    {
        //updateShake(deltaTime);
        //startShake(100.0f, 20.0f);
        //glViewport(
        //    static_cast<GLint>(shakeOffsetX),
        //    static_cast<GLint>(shakeOffsetY),
        //    static_cast<GLsizei>(projWidth),
        //    static_cast<GLsizei>(projHeight)
        //);
        // Bind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, gameFramebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        (void)deltaTime;
        // Render your scene, but output entity IDs as colors to the pickingFBO

        //  ------ Graphics Rendering Pipeline START -----
        models.clear();
        // -- Sort renderable entities on layer and ID
        //  if (CurrentSize != mEntities.size()) {
            // Populate the sortedEntities vector with elements from the mEntities set

        sortedEntities.assign(mEntities.begin(), mEntities.end());

        // Sort based on LayerID, then SortID, and finally Entity ID
        std::sort(sortedEntities.begin(), sortedEntities.end(), [](Entity a, Entity b) {
            auto& layerA = ecsInterface.GetComponent<LayerComponent>(a);
            auto& layerB = ecsInterface.GetComponent<LayerComponent>(b);

            // First, compare LayerID
            if (layerA.layerID != layerB.layerID) {
                return layerA.layerID < layerB.layerID;
            }

            // If LayerID is the same, compare SortID
            if (layerA.sortID != layerB.sortID) {
                return layerA.sortID < layerB.sortID;
            }

            // If both LayerID and SortID are the same, fall back to Entity ID
            return a < b;
            });

        // Update CurrentSize to reflect the new size of mEntities
        CurrentSize = static_cast<unsigned int>(mEntities.size());
        //  }

        for (auto const& entityId : sortedEntities)
        {
            // Get Components needed
            TransformComponent& transformComponent = ecsInterface.GetComponent<TransformComponent>(entityId);
            RenderComponent& renderComponent = ecsInterface.GetComponent<RenderComponent>(entityId);
            LayerComponent& layerComponent = ecsInterface.GetComponent<LayerComponent>(entityId);

 
            //Skip render base on visibility of layer
            if (!engineState.layerVisibility[layerComponent.layerID]) {
                continue;  // Skip all entities in this layer
            }

            
            if(ecsInterface.HasTag(entityId, "WinUI"))
            {
                if (engineState.IsWin())
                {
                    renderComponent.isActive = true; // player wins here. render the win screen

                    if (!hasAudioWin)
                    {
                        GlobalAudio.UE_BGM_Reset();
                        GlobalAudio.UE_PlaySound("Funkalicious", false);
                        hasAudioWin = true; // Mark as Win
                        hasAudioLose = false; // Reset lose flag
                    }
                }

                if(!engineState.IsWin())
                {
                    renderComponent.isActive = false;   // player hasn't won or lost here. nothing to render
                    hasAudioWin = false;                // Reset win flag when exiting win state
                }
            }

            if(ecsInterface.HasTag(entityId, "LoseUI"))
            {
                if (engineState.IsLose())
                {
                    renderComponent.isActive = true; // player loses here. render the lose screen

                    if (!hasAudioLose)
                    {
                        GlobalAudio.UE_BGM_Reset();
                        GlobalAudio.UE_PlaySound("Duskwalkin", false);
                        hasAudioLose = true;    // Mark as Lose
                        hasAudioWin = false;    // Reset win flag
                    }
                }

                if(!engineState.IsLose())
                {
                    renderComponent.isActive = false; // player hasn't won or lost here. nothing to render
                    hasAudioLose = false;       // Reset lose flag when exiting lose state
                }
            }
            
            if (!renderComponent.isActive) {
                continue; // ues this line to ensure its active before it operates
            }

            // Creating animation sprite in here //
            if (ecsInterface.HasComponent<AnimationComponent>(entityId)) {
                if (ecsInterface.HasComponent<CollisionComponent>(entityId)) {
                    CollisionComponent& playercol = ecsInterface.GetComponent<CollisionComponent>(entityId);
                    (void)playercol;
                }
                AnimationComponent& animationComponent = ecsInterface.GetComponent<AnimationComponent>(entityId);
                if (animationComponent.currentAnimation != renderComponent.textureID) {
                    animationComponent.currentAnimation = renderComponent.textureID;
                    animationComponent.animationTimeStart = static_cast<float>(glfwGetTime());  // Reset animation start time
                    animationComponent.currentFrame = 0;
                }
                Graphics::Model& modelanim = getMesh("animation");
                if (textures.find(renderComponent.textureID) == textures.end()) {
                    textures[renderComponent.textureID] = GlobalAssetManager.UE_LoadTextureToOpenGL(renderComponent.textureID);
                }
                modelanim.textureID = textures[renderComponent.textureID];
                glm::vec2 scale_anim(transformComponent.scale.x, transformComponent.scale.y);
                glm::vec2 transla(transformComponent.position.x, transformComponent.position.y);
                modelanim.modelMatrix = Graphics::calculate2DTransform(transla, 0.0f, scale_anim);
                float elapsedTime = static_cast<float>(glfwGetTime() - animationComponent.animationTimeStart);
                if (!engineState.IsPaused())
                {
                    animationComponent.currentFrame = (int)(elapsedTime * animationComponent.animationSpeed) % (animationComponent.rows * animationComponent.cols);
                }
                else
                {
                    animationComponent.currentFrame = (int)(animationComponent.animationSpeed) % (animationComponent.rows * animationComponent.cols);
                }
                
                
               // animationComponent.currentFrame = (int)(elapsedTime * animationComponent.animationSpeed) % (animationComponent.rows*animationComponent.cols);
                modelanim.alpha = renderComponent.alpha;
                modelanim.color = renderComponent.color;
                
                drawMeshWithAnimation(modelanim, animationComponent.currentFrame, animationComponent.cols, animationComponent.rows);
                modelanim.draw();
            }
            
            //check if they do not have animation component, this way render wont render over the animation
            if (!(ecsInterface.HasComponent<AnimationComponent>(entityId))) {
                if (ecsInterface.HasComponent<RenderComponent>(entityId)) {
                    // Sprite rendering
                    Graphics::Model& model = getMesh("sprite"); // Use for mesh

                    // Loads the texture if it�s not loaded into map
                    if (textures.find(renderComponent.textureID) == textures.end()) {
                        textures[renderComponent.textureID] = GlobalAssetManager.UE_LoadTextureToOpenGL(renderComponent.textureID);
                    }

                    model.textureID = textures[renderComponent.textureID]; // Assign loaded texture ID to model

                    // TRANSLATE, ROTATE, SCALE
                    glm::vec2 translation(transformComponent.position.x, transformComponent.position.y);
                    float rotation = transformComponent.rotation;
                    glm::vec2 scale(transformComponent.scale.x, transformComponent.scale.y);

                    // SET MATRIX
                    model.modelMatrix = Graphics::calculate2DTransform(translation, rotation, scale);

                    // Set color and alpha 
                    model.color = renderComponent.color;
                    model.alpha = renderComponent.alpha;

                    // Draw the model
                    model.draw();
                }
            }
            
            if (ecsInterface.HasComponent<UIBarComponent>(entityId)) {
                const UIBarComponent& barComponent = ecsInterface.GetComponent<UIBarComponent>(entityId);

                // === Bar position (backing) ===
                glm::vec2 barPos = transformComponent.position + barComponent.offset;

                // Get mesh
                Graphics::Model& model = getMesh("sprite");

                // === Draw Background Bar ===
                if (textures.find(barComponent.backingTextureID) == textures.end()) {
                    textures[barComponent.backingTextureID] = GlobalAssetManager.UE_LoadTextureToOpenGL(barComponent.backingTextureID);
                }
                model.textureID = textures[barComponent.backingTextureID];

                model.modelMatrix = Graphics::calculate2DTransform(barPos, 0.0f, barComponent.scale);
                model.color = glm::vec4(barComponent.bgColor, barComponent.bgAlpha);
                model.alpha = barComponent.bgAlpha;
                model.draw();

                // === Draw Fill Bar ===
                if (textures.find(barComponent.fillTextureID) == textures.end()) {
                    textures[barComponent.fillTextureID] = GlobalAssetManager.UE_LoadTextureToOpenGL(barComponent.fillTextureID);
                }
                model.textureID = textures[barComponent.fillTextureID];

                // Calculate filled size using fillSize instead of scale
                glm::vec2 filledSize(
                    barComponent.fillSize.x * barComponent.FillPercentage,
                    barComponent.fillSize.y
                );

                // Fill position = base barPos + fillOffset + half filled width (to center it properly)
                glm::vec2 fillPos = barPos + barComponent.fillOffset;
                fillPos.x += 0.5f * filledSize.x; // anchor to the left of fill area

                model.modelMatrix = Graphics::calculate2DTransform(fillPos, 0.0f, filledSize);
                model.color = glm::vec4(barComponent.fillColor, barComponent.fillAlpha);
                model.alpha = barComponent.fillAlpha;
                model.draw();
            }


            if (ecsInterface.HasComponent<TextComponent>(entityId)) {
                // Text rendering
                TextComponent& textComponent = ecsInterface.GetComponent<TextComponent>(entityId);

                // Set active font
                fontSystem.SetActiveFont(textComponent.fontName);
                // Calculate position based on TransformComponent + offset
                glm::vec2  textPosition = transformComponent.position + textComponent.offset;

                // Set up an orthographic projection for 2D text rendering
                glm::mat4 projection = glm::ortho(0.0f, projWidth, projHeight, 0.0f); // Example viewport size, adjust as needed
                //Print here

                // Render the text
                fontSystem.RenderText(
                    textComponent.text,
                    textPosition.x,
                    textPosition.y,
                    static_cast<float>(textComponent.fontSize),
                    textComponent.color,
                    projection
                );
            }

            if (ecsInterface.HasComponent<CollisionComponent>(entityId)) {
                if (engineState.IsInDebugMode()) {
                    CollisionComponent& collisionComponent = ecsInterface.GetComponent<CollisionComponent>(entityId);
                    // Renders debug box using collision component
                    Graphics::DrawDebugBox(transformComponent.position, collisionComponent.scale.x, collisionComponent.scale.y); // For example, drawing a debug box
                }
            }

            //Draw fps here
            if (engineState.IsDisplayFPS()) {
                RenderFPS(projWidth, projHeight);
            }
        }

        for (auto& model : models)
        {
            model.draw(); // Call the draw function on each model
        }
        if (Graphics::toggleImGUI == false)
        {
            //// Bind the default framebuffer again
            gameFramebuffer = 0;
        }
        else
        {
            // Start a new frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Create a full-screen dockspace
            ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            ImGuiViewport* viewport = ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            // Add flags to prevent moving and resizing
            windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

            // Begin the main dockspace window
            ImGui::Begin("DockSpace Demo", nullptr, windowFlags);
            ImGui::PopStyleVar(2);

            // Bind the default framebuffer again
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a dockspace ID
            ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");

            // Create the dockspace
            ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

            // Optionally add a menu bar
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Unnamed Studio Game Engine"))
                {
                    if (ImGui::MenuItem("Open"))
                    {
                        // Get the file path from the OpenFileDialog
                        filePath = Graphics::OpenFileDialog();

                        // Ensure the file path is not empty before proceeding
                        if (!filePath.empty())
                        {
                            // Store the current directory before changing
                            std::wstring originalDir = Graphics::GetCurrentWorkingDirectory();
                            std::wcout << L"Original Directory: " << originalDir << std::endl;

                            // Extract the directory of the scene file
                            std::string newDirectory = Graphics::ExtractBasePath(filePath);
                            std::cout << "New Directory (Extracted): " << newDirectory << std::endl;

                            // Change to the directory of the file
                            Graphics::ChangeWorkingDirectory(std::wstring(newDirectory.begin(), newDirectory.end()));
                            std::wcout << L"Changed Working Directory: "
                                << Graphics::GetCurrentWorkingDirectory() << std::endl;

                            // Load the entities
                            ecsInterface.ClearEntities();
                            entityAssets.clear();
                            GlobalAssetManager.UE_LoadEntities(filePath);
                            //GlobalAudio.UE_Reset();

                            // Revert to the directory two levels above the original directory
                            std::wstring parentDir = Graphics::ExtractParentDirectory(originalDir, 2);
                            Graphics::ChangeWorkingDirectory(parentDir);
                            std::wcout << L"Reverted to Parent's Parent Directory: "
                                << Graphics::GetCurrentWorkingDirectory() << std::endl;
                        }
                    }

                    if (ImGui::MenuItem("Save"))
                    {
                        std::string savePath = SaveFileDialog();
                        if (!savePath.empty())
                        {
                            GlobalEntityAsset.SerializeEntities(savePath);
                        }
                        else
                        {
                            std::cout << "Save path selected: " << savePath << std::endl;
                        }
                    }
                        // Blocking to remove confusion
                    if (ImGui::MenuItem("Load Prefab"))
                    {
                        // Get the file path from the OpenFileDialog
                        filePath = Graphics::OpenFileDialog();

                        // Ensure the file path is not empty before proceeding
                        if (!filePath.empty())
                        {
                            // Store the current directory before changing
                            std::wstring originalDir = Graphics::GetCurrentWorkingDirectory();
                            std::wcout << L"Original Directory: " << originalDir << std::endl;

                            // Extract the directory of the scene file
                            std::string newDirectory = Graphics::ExtractBasePath(filePath);
                            std::cout << "New Directory (Extracted): " << newDirectory << std::endl;

                            // Change to the directory of the file
                            Graphics::ChangeWorkingDirectory(std::wstring(newDirectory.begin(), newDirectory.end()));
                            std::wcout << L"Changed Working Directory: "
                                << Graphics::GetCurrentWorkingDirectory() << std::endl;

                            // Load the entities
                            GlobalAssetManager.UE_LoadEntities(filePath);
                            //GlobalAudio.UE_Reset();

                            // Revert to the directory two levels above the original directory
                            std::wstring parentDir = Graphics::ExtractParentDirectory(originalDir, 2);
                            Graphics::ChangeWorkingDirectory(parentDir);
                            std::wcout << L"Reverted to Parent's Parent Directory: "
                                << Graphics::GetCurrentWorkingDirectory() << std::endl;
                        }
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            // End the dockspace window
            ImGui::End();

            //for edwin
            RenderErrorPopup();

            if (ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                if (ImGui::CollapsingHeader("Sprites", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    float iconSize = 64.0f;  // Size of each icon
                    float padding = 16.0f;   // Padding between icons
                    float cellSize = iconSize + padding;  // Total cell size
                    float windowWidth = ImGui::GetContentRegionAvail().x;  // Available window width
                    int columns = (int)(windowWidth / cellSize);  // Calculate number of columns
                    if (columns < 1) columns = 1;

                    ImGui::Columns(columns, nullptr, false);  // Set up columns

                    static char newNameBuffer[256] = { 0 };  // Buffer for in-place name editing
                    std::vector<std::pair<std::string, std::string>> nameUpdates;  // Vector for name updates
                    std::vector<std::string> texturesToDelete;  // Vector for textures marked for deletion

                    for (auto& texturePair : textureAssets)
                    {
                        const std::string& assetName = texturePair.first;
                        GLuint textureID = GlobalAssetManager.UE_LoadTextureToOpenGL(assetName);
                        ImTextureID imguiTextureID = static_cast<ImTextureID>(textureID);

                        ImGui::PushID(assetName.c_str());  // Ensure unique ID
                        ImGui::BeginGroup();  // Group icon and label

                        // Display the texture icon
                        ImGui::Image(imguiTextureID, ImVec2(iconSize, iconSize));

                        // Highlight selected texture with a border
                        if (selectedTextureName == assetName)
                        {
                            ImGui::GetWindowDrawList()->AddRect(
                                ImGui::GetItemRectMin(),
                                ImGui::GetItemRectMax(),
                                IM_COL32(255, 255, 0, 255)  // Yellow border
                            );
                        }

                        // Draw hover outline
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::GetWindowDrawList()->AddRect(
                                ImGui::GetItemRectMin(),
                                ImGui::GetItemRectMax(),
                                IM_COL32(0, 255, 0, 255),  // Green border for hover
                                4.0f,                      // Rounding
                                ImDrawFlags_RoundCornersAll, // Rounded corners
                                2.0f                       // Line thickness
                            );
                        }

                        // Right-click context menu for "Edit Name" and "Delete"
                        if (ImGui::BeginPopupContextItem(("ContextMenu_" + assetName).c_str()))
                        {
                            if (ImGui::MenuItem("Edit Name"))
                            {
                                selectedTextureName = assetName;  // Enable editing for this texture
                                strncpy(newNameBuffer, assetName.c_str(), sizeof(newNameBuffer));  // Pre-fill with current name
                            }

                            if (ImGui::MenuItem("Delete"))
                            {
                                texturesToDelete.push_back(assetName);  // Mark for deletion
                            }
                            ImGui::EndPopup();
                        }

                        // Display name as editable InputText if selected for editing
                        if (selectedTextureName == assetName)
                        {
                            if (ImGui::InputText("##edit", newNameBuffer, sizeof(newNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                std::string newName = newNameBuffer;
                                if (!newName.empty() && newName != assetName)
                                {
                                    nameUpdates.emplace_back(assetName, newName);  // Schedule name update
                                    selectedTextureName = "";  // Clear selection
                                }
                            }

                            // Exit editing mode if the user clicks outside
                            if (ImGui::IsItemDeactivated())
                            {
                                selectedTextureName = "";
                            }
                        }
                        else
                        {
                            // Display the texture name
                            ImGui::TextWrapped("%s", assetName.c_str());
                        }

                        ImGui::EndGroup();  // End icon-label grouping
                        ImGui::NextColumn();  // Move to the next column
                        ImGui::PopID();
                    }

                    // Apply name updates
                    for (const auto& [oldName, newName] : nameUpdates)
                    {
                        GlobalAssetManager.UE_UpdateTextureName(oldName, newName);
                        std::cout << "Renamed texture: " << oldName << " to " << newName << std::endl;
                    }
                    nameUpdates.clear();

                    // Apply deletions
                    for (const auto& textureName : texturesToDelete)
                    {
                        GlobalAssetManager.UE_DeleteTexture(textureName);
                        std::cout << "Deleted texture: " << textureName << std::endl;
                    }
                    texturesToDelete.clear();

                    ImGui::Columns(1);  // Reset columns
                }

                ImGui::NewLine();

                // Audio section
                if (ImGui::CollapsingHeader("Audio"))
                {
                    for (const auto& audioPair : audioAssets)
                    {
                        const std::string& assetName = audioPair.first;

                        // Check if this audio asset is being edited
                        bool& isEditingThisName = editingStates[assetName];

                        // Ensure unique IDs for ImGui elements
                        std::string uniqueID = "##RenameAudio_" + assetName;

                        // Check if this is the currently selected asset
                        bool isSelected = (selectedAudioName == assetName);

                        // If this is the selected asset, handle renaming or editing display
                        if (isSelected && isEditingThisName)
                        {
                            // Input field for renaming the asset
                            strncpy(newAudioNameBuffer, assetName.c_str(), sizeof(newAudioNameBuffer) - 1);
                            newAudioNameBuffer[sizeof(newAudioNameBuffer) - 1] = '\0';

                            if (ImGui::InputText(uniqueID.c_str(), newAudioNameBuffer, sizeof(newAudioNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                std::string newAudioName(newAudioNameBuffer);
                                if (!newAudioName.empty() && newAudioName != assetName)
                                {
                                    // Debug output for renaming
                                    std::cout << "Renaming from " << assetName << " to " << newAudioName << std::endl;

                                    // Perform the rename operation
                                    GlobalAssetManager.UE_UpdateAudioName(assetName, newAudioName);

                                    // Update the selected name and exit editing mode
                                    selectedAudioName = newAudioName;
                                    isEditingThisName = false;
                                }
                            }
                        }
                        else
                        {
                            // Render the selectable item for this asset
                            if (ImGui::Selectable((assetName + uniqueID).c_str(), isSelected))
                            {
                                selectedAudioName = assetName; // Set as selected
                                isEditingThisName = false;     // Disable edit mode when selected
                            }

                            // Handle double-click to enter editing mode
                            if (isSelected && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                            {
                                isEditingThisName = true; // Enter edit mode
                            }
                        }

                        // Handle deleting the audio asset using a keyboard shortcut (Delete key)
                        if (selectedAudioName == assetName && ImGui::IsKeyPressed(ImGuiKey_Delete))
                        {
                            ImGui::OpenPopup(("Confirm Delete?##" + assetName).c_str());
                        }

                        // Optionally show a confirmation popup for deletion
                        if (ImGui::BeginPopupModal(("Confirm Delete?##" + assetName).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
                        {
                            ImGui::Text("Are you sure you want to delete this audio asset?");
                            if (ImGui::Button("Yes"))
                            {
                                audioToDelete.push_back(assetName); // Mark for deletion
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("No"))
                            {
                                ImGui::CloseCurrentPopup(); // Close popup without deleting
                            }
                            ImGui::EndPopup();
                        }

                        // Right-click to open the context menu
                        if (ImGui::BeginPopupContextItem(("ContextMenu_" + assetName).c_str()))  // Right-click to open the context menu
                        {
                            if (ImGui::MenuItem("Delete"))
                            {
                                audioToDelete.push_back(assetName);  // Mark for deletion
                            }

                            ImGui::EndPopup();
                        }
                    }

                    // Perform deletions after the loop to avoid iterator invalidation
                    for (const auto& name : audioToDelete)
                    {
                        GlobalAssetManager.UE_DeleteAudio(name);

                        // Only reset selectedAudioName if the deleted asset was the selected one
                        if (selectedAudioName == name)
                        {
                            selectedAudioName.clear(); // Reset selection if necessary
                        }
                    }
                    audioToDelete.clear();
                }
                else
                {
                    // Reset all editing states when the node is collapsed
                    editingStates.clear();
                    selectedAudioName.clear();
                }

                if (!selectedAudioName.empty())
                {
                    // Check if the audio selection has changed
                    if (previousSelectedAudioName != selectedAudioName)
                    {
                        // Stop or reset the previous audio
                        if (!previousSelectedAudioName.empty())
                        {
                            GlobalAudio.UE_Reset();
                        }

                        // Update the previous selection
                        previousSelectedAudioName = selectedAudioName;
                    }

                    // Play button
                    if (ImGui::Button("Play"))
                    {
                        GlobalAudio.UE_PlaySound(selectedAudioName, false); // Play the selected audio
                    }

                    // Pause/Resume button
                    ImGui::SameLine();
                    if (ImGui::Button("Pause/Resume"))
                    {
                        GlobalAudio.UE_PauseSound(selectedAudioName); // Pause/Resume the selected audio
                    }

                    // Reset button
                    ImGui::SameLine();
                    if (ImGui::Button("Reset"))
                    {
                        GlobalAudio.UE_Reset(); // Reset audio playback
                    }

                    // Dropdown Box Mode
                    ImGui::Text("Select Mode Option: ");

                    // Retrieve the AudioAsset using the AssetManager
                    auto audioAsset = GlobalAssetManager.UE_GetAudioAsset(selectedAudioName); // This returns a pointer to the AudioAsset

                    static int selectedOption1 = 0; // This will hold the selected index
                    std::vector<std::string> modeOptions;

                    static int selectedSoundTypeIndex = 0; // This will hold the selected index
                    std::vector<std::string> soundTypeOptions;

                    if (audioAsset)  // Check if audioAsset is valid
                    {
                        try
                        {
                            // Get the current mode for the selected audio asset
                            std::string currentMode = GlobalAssetManager.UE_GetMusicMode(selectedAudioName);

                            // Clear previous options to prevent duplication
                            modeOptions.clear();

                            // Add the current mode as the first option and the other mode as the second
                            if (currentMode == "oneshot")
                            {
                                modeOptions.push_back("oneshot");  // Current mode
                                modeOptions.push_back("loop");     // Other mode
                            }
                            else if (currentMode == "loop")
                            {
                                modeOptions.push_back("loop");     // Current mode
                                modeOptions.push_back("oneshot");  // Other mode
                            }
                            else
                            {
                                modeOptions.push_back("Unknown");  // Placeholder for an unknown mode
                                modeOptions.push_back("oneshot");
                                modeOptions.push_back("loop");
                            }

                            // Use ImGui::Combo to allow the user to select an option
                            static std::string selectedMode = currentMode;  // Initialize with the current mode

                            if (selectedMode != currentMode)
                            {
                                selectedMode = currentMode;  // Sync selectedMode with currentMode
                            }

                            if (ImGui::BeginCombo("##Mode", selectedMode.c_str()))
                            {
                                for (const auto& mode : modeOptions)
                                {
                                    bool isSelected = (selectedMode == mode);  // Check if the mode matches the selected mode
                                    if (ImGui::Selectable(mode.c_str(), isSelected))
                                    {
                                        selectedMode = mode;  // Update the selected mode with the string value
                                        modeChanged = true;    // Mark that the mode has changed
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            // Handle the mode change
                            if (modeChanged)
                            {
                                std::string newMode = selectedMode;

                                if (newMode != currentMode)
                                {
                                    auto audioMap = audioAssets.find(selectedAudioName);
                                    if (audioMap != audioAssets.end())
                                    {
                                        AudioAsset::MusicAsset* musicAsset = GlobalAssetManager.UE_GetMusicAssetByName(selectedAudioName);

                                        if (musicAsset != nullptr)
                                        {
                                            // Update the mode
                                            musicAsset->mode = newMode;
                                            std::cout << "Updated " << selectedAudioName << " mode to " << newMode << std::endl;

                                            // Serialize the updated audio asset
                                            AudioAsset::SerializeAudio("Assets/JsonData/AudioAsset.json", audioAssets);
                                            std::cout << "Audio asset serialized after mode update." << std::endl;

                                            // Reset flags after updating
                                            modeChanged = false;
                                        }
                                        else
                                        {
                                            std::cerr << "MusicAsset not found for audio name: " << selectedAudioName << std::endl;
                                        }
                                    }
                                }
                                GlobalAudio.UE_Reset();
                            }
                        }
                        catch (const std::runtime_error& e)
                        {
                            // Handle error if the mode retrieval fails
                            ImGui::Text("Error: %s", e.what());
                        }
                    }
                    else
                    {
                        ImGui::Text("Error: AudioAsset not found.");
                    }

                    // Dropdown Box Sound Type
                    ImGui::Text("Select Sound Type: ");

                    if (audioAsset)  // Check if audioAsset is valid
                    {
                        try
                        {
                            // Get the current SoundType for the selected audio asset
                            Framework::Audio::SoundType soundType = GlobalAssetManager.UE_GetMusicSoundType(selectedAudioName);

                            // Clear previous options to prevent duplication
                            soundTypeOptions.clear();

                            // Add available sound types to the options list
                            soundTypeOptions.push_back("Background_Music");
                            soundTypeOptions.push_back("Sound_Effect");
                            soundTypeOptions.push_back("Empty");

                            // Initialize selectedSoundTypeIndex with the index of the current sound type
                            if (soundType == Framework::Audio::SoundType::BACKGROUND_MUSIC)
                            {
                                selectedSoundTypeIndex = 0;
                            }
                            else if (soundType == Framework::Audio::SoundType::SOUND_EFFECT)
                            {
                                selectedSoundTypeIndex = 1;
                            }
                            else if (soundType == Framework::Audio::SoundType::EMPTY)
                            {
                                selectedSoundTypeIndex = 2;
                            }

                            // Create the combo box to allow the user to select a sound type
                            if (ImGui::BeginCombo("##SoundType", soundTypeOptions[selectedSoundTypeIndex].c_str()))
                            {
                                for (int i = 0; i < soundTypeOptions.size(); ++i)
                                {
                                    bool isSelected = (selectedSoundTypeIndex == i);
                                    if (ImGui::Selectable(soundTypeOptions[i].c_str(), isSelected))
                                    {
                                        selectedSoundTypeIndex = i;  // Update the selected sound type
                                        soundTypeChanged = true;     // Mark that the sound type has changed
                                    }
                                }
                                ImGui::EndCombo();
                            }

                            // Handle the sound type change
                            if (soundTypeChanged)
                            {
                                Framework::Audio::SoundType newSoundType;
                                switch (selectedSoundTypeIndex)
                                {
                                case 0: newSoundType = Framework::Audio::SoundType::BACKGROUND_MUSIC; break;
                                case 1: newSoundType = Framework::Audio::SoundType::SOUND_EFFECT; break;
                                case 2: newSoundType = Framework::Audio::SoundType::EMPTY; break;
                                default: return;  // Invalid index, do nothing
                                }

                                // Update the sound type in the music asset
                                auto audioMap = audioAssets.find(selectedAudioName);
                                if (audioMap != audioAssets.end())
                                {
                                    //auto& audioAsset = audioMap->second;

                                    AudioAsset::MusicAsset* musicAsset = GlobalAssetManager.UE_GetMusicAssetByName(selectedAudioName);

                                    if (musicAsset != nullptr)
                                    {
                                        // Update the sound type
                                        musicAsset->soundType = newSoundType;
                                        std::cout << "Updated " << selectedAudioName << " sound type to " << soundTypeOptions[selectedSoundTypeIndex] << std::endl;

                                        // Serialize the updated audio asset
                                        AudioAsset::SerializeAudio("Assets/JsonData/AudioAsset.json", audioAssets);

                                        std::cout << "Audio asset serialized after sound type update." << std::endl;

                                        // Reset flags after updating
                                        soundTypeChanged = false;
                                    }
                                    else
                                    {
                                        std::cerr << "MusicAsset not found for audio name: " << selectedAudioName << std::endl;
                                    }
                                }
                            }
                        }
                        catch (const std::runtime_error& e)
                        {
                            // Handle error if the sound type retrieval fails
                            ImGui::Text("Error: %s", e.what());
                        }
                    }
                    else
                    {
                        ImGui::Text("Error: AudioAsset not found.");
                    }

                    // Dropdown Box for Audio Path Selection (BGM or SoundEffect)
                    ImGui::Text("Select Audio Path: ");

                    if (audioAsset)  // Check if audioAsset is valid
                    {
                        try
                        {
                            // Get the current audio path for the selected audio asset inside the try block
                            std::string currentPath = GlobalAssetManager.UE_GetMusicFilePath(selectedAudioName);

                            // Remove the file extension from the current path (to get just the folder path)
                            size_t lastSlash = currentPath.find_last_of("/\\"); // Find the last slash in the path
                            std::string folderPath = (lastSlash != std::string::npos) ? currentPath.substr(0, lastSlash + 1) : currentPath;

                            // Extract the file extension from the current path (full file path)
                            size_t dotPos = currentPath.find_last_of(".");
                            std::string fileExtension = (dotPos != std::string::npos) ? currentPath.substr(dotPos + 1) : "";

                            // Check which path is currently selected and set it
                            for (int i = 0; i < audioPathOptions.size(); ++i)
                            {
                                if (audioPathOptions[i] == folderPath)
                                {
                                    selectedAudioPathIndex = i;  // Set to the current path index
                                    break;
                                }
                            }

                            // Create the combo box for selecting the audio path
                            if (!audioPathOptions.empty())
                            {
                                // Convert audioPathOptions to const char* array for ImGui
                                std::vector<const char*> audioPathCStrOptions;
                                for (const auto& path : audioPathOptions)
                                {
                                    audioPathCStrOptions.push_back(path.c_str());
                                }

                                // Display the Combo Box for selecting the audio path
                                if (ImGui::Combo("##AudioPath", &selectedAudioPathIndex, audioPathCStrOptions.data(), static_cast<int>(audioPathCStrOptions.size())))
                                {
                                    // Get the new path selected by the user
                                    std::string newPath = audioPathOptions[selectedAudioPathIndex];

                                    // Handle the path change only if the new path is different from the current one
                                    if (newPath != folderPath)
                                    {
                                        // Build the new and old full paths
                                        std::string newFullPath = newPath + selectedAudioName + "." + fileExtension;
                                        std::string oldFullPath = folderPath + selectedAudioName + "." + fileExtension;

                                        // Copy the file to the new folder
                                        if (GlobalAssetManager.UE_CopyAudioToFolder(oldFullPath, newPath))
                                        {
                                            // Delete the original file
                                            if (!GlobalAssetManager.UE_DeleteAudioFile(oldFullPath))
                                            {
                                                std::cerr << "Warning: Failed to delete the original file after copying." << std::endl;
                                            }

                                            // Update the file path in the asset
                                            auto audioMap = audioAssets.find(selectedAudioName);
                                            if (audioMap != audioAssets.end())
                                            {
                                                //auto& audioAsset = audioMap->second;
                                                AudioAsset::MusicAsset* musicAsset = GlobalAssetManager.UE_GetMusicAssetByName(selectedAudioName);

                                                if (musicAsset != nullptr)
                                                {
                                                    musicAsset->filePath = newFullPath; // Update with the new full path
                                                    std::cout << "Updated " << selectedAudioName << " file path to " << musicAsset->filePath << std::endl;

                                                    // Serialize the updated audio asset
                                                    AudioAsset::SerializeAudio("Assets/JsonData/AudioAsset.json", audioAssets);
                                                    std::cout << "Audio asset serialized after file path update." << std::endl;
                                                }
                                                else
                                                {
                                                    std::cerr << "MusicAsset not found for audio name: " << selectedAudioName << std::endl;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            std::cerr << "Error: Failed to copy the audio file to the new location." << std::endl;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                ImGui::Text("No audio paths available.");
                            }
                        }
                        catch (const std::runtime_error& e)
                        {
                            // Handle error if the audio path retrieval fails
                            ImGui::Text("Error: %s", e.what());
                        }
                    }
                    else
                    {
                        ImGui::Text("Error: AudioAsset not found.");
                    }
                }
                ImGui::End();
            }

            // "Global variables" For Victor and Joel for IMGUI portion
            static int selectedModelIndex = -1;
            static char meshNameBuffer[128] = "";  // Buffer for the mesh name
            static char imagePathBuffer[256] = "";  // Buffer for the image path

            if (ImGui::Begin("Entity", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                if (ImGui::CollapsingHeader("Entity Creation", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::Button("Create Entity", ImVec2(140, 30)))
                    {
                        // Call to ECS Interface to create a new entity
                        //Create default transform to assign new entity to centre of screen with visible scaling.
                        TransformComponent DefaultTransform{};
                        DefaultTransform.position = { projWidth / 2, projHeight / 2 };
                        DefaultTransform.scale = { 100.f,100.f };

                        Entity newEntity = ecsInterface.CreateEntity();
                        ecsInterface.AddComponent<TransformComponent>(newEntity, DefaultTransform);
                        ecsInterface.AddComponent<RenderComponent>(newEntity, RenderComponent{});
                        ecsInterface.AddComponent<LayerComponent>(newEntity, LayerComponent{});
                        ecsInterface.AddComponent<TextComponent>(newEntity, TextComponent{});
                        std::cout << "Entity created with ID: " << newEntity << std::endl;
                        screenShake = true;
                    }


                    if (ImGui::Button("Delete All Entity", ImVec2(140, 30)))
                    {
                        ImGui::OpenPopup("Confirm Clear All"); // Open the confirmation popup
                    }

                    // Center the modal
                    ImVec2 center = ImGui::GetMainViewport()->GetCenter(); // Get the center of the viewport
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f)); // Center modal

                    // Create the popup modal
                    if (ImGui::BeginPopupModal("Confirm Clear All", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("Are you sure?");
                        ImGui::Separator();

                        if (ImGui::Button("Yes", ImVec2(120, 0))) // Confirm button
                        {
                            isPropertiesWindowOpen = false;
                            // Clear all objects and reset fields
                            ecsInterface.ClearEntities();
                            models.clear();

                            ImGui::CloseCurrentPopup(); // Close the popup
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Cancel", ImVec2(120, 0))) // Cancel button
                        {
                            ImGui::CloseCurrentPopup(); // Close the popup without clearing
                        }

                        ImGui::EndPopup();
                    }

                    if (ImGui::Button("Delete Entity", ImVec2(140, 30)))
                    {
                        ImGui::OpenPopup("Confirm Clear Entity"); // Open the confirmation popup
                    }

                    // Create the popup modal
                    if (ImGui::BeginPopupModal("Confirm Clear Entity", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("Are you sure?");
                        ImGui::Separator();

                        if (ImGui::Button("Yes", ImVec2(120, 0))) // Confirm button
                        {
                            if (selectedEntity != std::numeric_limits<Entity>::max())
                            {
                                ecsInterface.DestroyEntity(selectedEntity);
                            }
                            ImGui::CloseCurrentPopup(); // Close the popup
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Cancel", ImVec2(120, 0))) // Cancel button
                        {
                            ImGui::CloseCurrentPopup(); // Close the popup without clearing
                        }

                        ImGui::EndPopup();
                    }
                }

                ImGui::Begin("Entity List");
                // Display each entity as a selectable item
                for (Entity entity : ecsInterface.GetEntities())
                {
                    std::string entityName = ecsInterface.GetEntityName(entity);

                    // Highlight the selected entity
                    if (ImGui::Selectable(entityName.c_str(), selectedEntity == entity))
                    {
                        selectedEntity = entity;  // Update selected entity
                        isPropertiesWindowOpen = true;  // Open properties window
                    }
                }
                ImGui::End();

                // Separate panel to edit the selected entity's name
                if (isPropertiesWindowOpen && selectedEntity != std::numeric_limits<Entity>::max())
                {

                    if (!ecsInterface.IsEntityValid(selectedEntity))
                    {
                        isPropertiesWindowOpen = false;
                        selectedEntity = std::numeric_limits<Entity>::max(); // Deselect the entity
                    }
                    else
                    {
                        auto entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // gets the signature of the selected entity.
                        //  std::cout << entitySignature << std::endl;
                        auto inverseSignature = ~entitySignature;                                // Has the flipped signature of the selected entity's signature. This is for component adding features
                        bool hasAnyComponent = entitySignature.any();                           // returns true if the entity has at least 1 component. Basically if the 32 bit signature has at least a 1 amongst all the bits

                        ImGui::Begin("Entity Properties", &isPropertiesWindowOpen);

                        // Displays name editor under the entity properties for all entities. Name isn't a component. Does not require any form of checks to appear
                        if (ImGui::CollapsingHeader("Name", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            std::string selectedEntityName = ecsInterface.GetEntityName(selectedEntity);
                            char buffer[256];
                            strncpy(buffer, selectedEntityName.c_str(), sizeof(buffer));
                            buffer[sizeof(buffer) - 1] = '\0';

                            // Display an InputText to edit the selected entity's name
                            if (ImGui::InputText("Edit Name", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                ecsInterface.SetEntityName(selectedEntity, std::string(buffer));  // Update the name of the selected entity
                            }

                            ImGui::Spacing();
                            ImGui::Spacing();

                            // Display tag, utilizes TransformComponent (Default included) with safe checks.
                            if (ecsInterface.HasComponent<TransformComponent>(selectedEntity))
                            {
                                TransformComponent& transformComponent = ecsInterface.GetComponent<TransformComponent>(selectedEntity);

                                static char tagBuffer[128];  // Persistent Buffer for ImGui input

                                // **Only copy the tag from component when entity selection changes**
                                static int lastSelectedEntity = -1;  // Track entity selection
                                if (lastSelectedEntity != static_cast<int>(selectedEntity))
                                {
                                    strncpy(tagBuffer, transformComponent.tag.c_str(), sizeof(tagBuffer)); // Copy current tag into buffer
                                    tagBuffer[sizeof(tagBuffer) - 1] = '\0';  // Ensure null termination
                                    lastSelectedEntity = selectedEntity;  // Update tracker
                                }

                                ImGui::Text("Entity Tags");
                                ImGui::InputText("##TagInput", tagBuffer, sizeof(tagBuffer));

                                ImGui::SameLine();
                                if (ImGui::Button("Add Tag"))
                                {
                                    transformComponent.tag = std::string(tagBuffer);  // Update tag only when button is clicked
                                    std::string tagString = transformComponent.tag;

                                    // **Step 1: Remove all spaces**
                                    tagString.erase(remove_if(tagString.begin(), tagString.end(), ::isspace), tagString.end());

                                    // **Step 2: Split tags by comma**
                                    std::stringstream ss(tagString);
                                    std::string tag;

                                    while (getline(ss, tag, ','))  // Split by comma
                                    {
                                        if (!tag.empty()) // Avoid adding empty tags
                                        {
                                            ecsInterface.AddTag(selectedEntity, tag);

                                        }
                                    }
                                }

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();

                                // **Display all tags of the entity, separated by commas**

                                std::unordered_set<std::string> entityTags = ecsInterface.GetTagsOfEntity(selectedEntity);

                                std::string tagList;
                                for (const auto& tag : entityTags)
                                {
                                    if (!tagList.empty()) tagList += ","; // Add a comma between tags
                                    tagList += tag;
                                }

                                ImGui::Text("Current Tags:%s", tagList.c_str());
                            }

                        }

                        // Breathing space between each component
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::CollapsingHeader("Global Layer Control", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            static int selectedGlobalLayer = 0; // Store selected layer for global control

                            // Dropdown for selecting which layer to toggle globally
                            const char* layerNames[] = { "Background", "Character", "Foreground", "UI", "Debug" };
                            ImGui::Combo("Global Layer", &selectedGlobalLayer, layerNames, IM_ARRAYSIZE(layerNames));

                            // Get current visibility status of the selected global layer
                            bool isGlobalLayerVisible = engineState.layerVisibility[static_cast<Layer>(selectedGlobalLayer)];

                            // Button to toggle visibility of the selected global layer
                            if (ImGui::Button(isGlobalLayerVisible ? "Disable Layer" : "Enable Layer"))
                            {
                                engineState.layerVisibility[static_cast<Layer>(selectedGlobalLayer)] = !isGlobalLayerVisible;
                            }

                            // Show visibility status of selected global layer
                            ImGui::SameLine();
                            ImGui::Text(isGlobalLayerVisible ? "[Visible]" : "[Hidden]");
                        }

                        // Breathing space between each component
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // If the selected has any component, then we test its signature to show the components it has.
                        if (hasAnyComponent)
                        {
                            // Checks if there is MovementComponent. If yes, show its variables
                            if (entitySignature.test(0))    // MovementComponent
                            {
                                MovementComponent& movementComponent = ecsInterface.GetComponent<MovementComponent>(selectedEntity);

                                if (ImGui::CollapsingHeader("Movement Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    ImGui::Text("Base Velocity:");

                                    // Store previous values before modification
                                    glm::vec2 prevBaseVelocity = movementComponent.baseVelocity;

                                    // Modify X velocity
                                    if (ImGui::InputFloat("Base X", &movementComponent.baseVelocity.x))
                                    {
                                        // Only push an undo action if the value actually changed
                                        if (prevBaseVelocity.x != movementComponent.baseVelocity.x)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "MovementComponent", "baseVelocity.x", movementComponent.baseVelocity.x, prevBaseVelocity.x, movementComponent.baseVelocity.x);
                                        }
                                    }

                                    // Modify Y velocity
                                    if (ImGui::InputFloat("Base Y", &movementComponent.baseVelocity.y))
                                    {
                                        // Only push an undo action if the value actually changed
                                        if (prevBaseVelocity.y != movementComponent.baseVelocity.y)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "MovementComponent", "baseVelocity.y", movementComponent.baseVelocity.y, prevBaseVelocity.y, movementComponent.baseVelocity.y);
                                        }
                                    }

                                    if (ImGui::Button("Remove Movement Component"))
                                    {
                                        inverseSignature.flip(0);
                                        
                                        // Push undo before removing the component
                                        undoRedoManager.PushUndoComponent(selectedEntity, movementComponent);

                                        ecsInterface.RemoveComponent<MovementComponent>(selectedEntity);
                                        entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // Fetch updated signature
                                    }
                                }

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(0);    // Sets the signature for MovementComponent to be 1. Means the selected entity doesn't have.
                            }

                            // Checks if there is EnemyComponent. If yes, show its variables
                            if (entitySignature.test(1))    // EnemyComponent
                            {
                                EnemyComponent& enemyComponent = ecsInterface.GetComponent<EnemyComponent>(selectedEntity);

                                if (ImGui::CollapsingHeader("Enemy Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    // Store initial values before editing
                                    float prevHealth = enemyComponent.health;
                                    float prevSpawnRate = enemyComponent.spawnRate;

                                    // Health Slider
                                    if (ImGui::SliderFloat("Health", &enemyComponent.health, 0.0f, 100.0f))
                                    {
                                        // If the value changed, push undo action
                                        undoRedoManager.PushUndo(selectedEntity, "EnemyComponent", "health", enemyComponent.health, prevHealth, enemyComponent.health);
                                    }

                                    // Spawn Rate Input
                                    if (ImGui::InputFloat("Spawnrate", &enemyComponent.spawnRate))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "EnemyComponent", "spawnRate", enemyComponent.spawnRate, prevSpawnRate, enemyComponent.spawnRate);
                                    }

                                    // Retrieve all registered behavior function names dynamically
                                    std::vector<std::string> behaviorOptions;
                                    for (const auto& pair : GlobalLogicManager.GetAllRegisteredEnemyFunctions()) 
                                    {
                                        behaviorOptions.push_back(pair.first); // Store function names
                                    }

                                    // Store initial function name
                                    std::string prevFunctionName = enemyComponent.UpdateFunctionName;

                                    if (ImGui::BeginCombo("Update Function", enemyComponent.UpdateFunctionName.empty() ? "Select Behavior" : enemyComponent.UpdateFunctionName.c_str()))
                                    {
                                        for (const auto& behaviorName : behaviorOptions)
                                        {
                                            bool isSelected = (enemyComponent.UpdateFunctionName == behaviorName);

                                            if (ImGui::Selectable(behaviorName.c_str(), isSelected))
                                            {
                                                enemyComponent.UpdateFunctionName = behaviorName; // Update selected behavior
                                                
                                                // Push undo only if it actually changed
                                                if (prevFunctionName != enemyComponent.UpdateFunctionName)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "EnemyComponent", "UpdateFunctionName", enemyComponent.UpdateFunctionName, prevFunctionName, enemyComponent.UpdateFunctionName);
                                                }
                                            }

                                            if (isSelected)
                                            {
                                                ImGui::SetItemDefaultFocus(); // Auto-scroll to selected item
                                            }
                                        }
                                        ImGui::EndCombo();
                                    }
                                    // Debugging display to confirm the selected function name
                                    ImGui::Text("Selected Behavior: %s", enemyComponent.UpdateFunctionName.c_str());
                                }

                                if (ImGui::Button("Remove Enemy Component"))
                                {
                                    // Store full EnemyComponent before removal
                                    undoRedoManager.PushUndoComponent(selectedEntity, enemyComponent);

                                    inverseSignature.flip(1);
                                    if (ecsInterface.HasComponent<MovementComponent>(selectedEntity)) {
                                        MovementComponent& movement = ecsInterface.GetComponent<MovementComponent>(selectedEntity);
                                        movement.velocity.x = 0;
                                        movement.velocity.y = 0;
                                    }
                                    ecsInterface.RemoveComponent<EnemyComponent>(selectedEntity);
                                    entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // Fetch updated signature
                                }
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(1);    // Sets the signature for EnemyComponent to be 1. Means the selected entity doesn't have.
                            }

                            // Checks if there is CollisionComponent. If yes, show its variables
                            // IDK if we need to show anything now??? But definitely need to be able to edit radius if the scale of the object is different
                            if (entitySignature.test(2))    // CollisionComponent
                            {
                                CollisionComponent& collisionComponent = ecsInterface.GetComponent<CollisionComponent>(selectedEntity);

                                if (ImGui::CollapsingHeader("Collision Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    float prevScaleX = collisionComponent.scale.x;  // Save the previous value
                                    float prevScaleY = collisionComponent.scale.y;  // Save the previous value
                                    ImGui::Text("Collision Size:");
                                    if (ImGui::InputFloat("X", &collisionComponent.scale.x))
                                    {
                                        // If the value changes, push the undo action
                                        if (prevScaleX != collisionComponent.scale.x)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "CollisionComponent", "scale.x", collisionComponent.scale.x, prevScaleX, collisionComponent.scale.x);
                                        }
                                    }
                                    if (ImGui::InputFloat("Y", &collisionComponent.scale.y))
                                    {
                                        // If the value changes, push the undo action
                                        if (prevScaleY != collisionComponent.scale.y)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "CollisionComponent", "scale.y", collisionComponent.scale.y, prevScaleY, collisionComponent.scale.y);
                                        }
                                    }
                                }

                                if (ImGui::Button("Remove Collision Component"))
                                {
                                    inverseSignature.flip(2);
                                    undoRedoManager.PushUndoComponent(selectedEntity, collisionComponent);
                                    ecsInterface.RemoveComponent<CollisionComponent>(selectedEntity);
                                    entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // Fetch updated signature
                                }
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(2);    // Sets the signature for CollisionComponent to be 1. Means the selected entity doesn't have.
                            }

                            // Checks if there is AnimationComponent. If yes, show its variables
                            if (entitySignature.test(3))    // AnimationComponent
                            {
                                AnimationComponent& animationComponent = ecsInterface.GetComponent<AnimationComponent>(selectedEntity);

                                if (ImGui::CollapsingHeader("Animation Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    // Store previous values for undo-redo
                                    float prevAnimationSpeed = animationComponent.animationSpeed;
                                    int prevRows = animationComponent.rows;
                                    int prevColumns = animationComponent.cols;

                                    if (ImGui::InputFloat("Animation Speed", &animationComponent.animationSpeed))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "Animation Component", "animation speed", animationComponent.animationSpeed, prevAnimationSpeed, animationComponent.animationSpeed);
                                    }

                                    if (ImGui::InputInt("Rows", &animationComponent.rows))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "Animation Component", "rows", animationComponent.rows, prevRows, animationComponent.rows);
                                    }

                                    if (ImGui::InputInt("Columns", &animationComponent.cols))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "Animation Component", "rows", animationComponent.cols, prevColumns, animationComponent.cols);
                                    }
                                }

                                if (ImGui::Button("Remove Animation Component"))
                                {
                                    undoRedoManager.PushUndoComponent(selectedEntity, animationComponent);
                                    ecsInterface.RemoveComponent<AnimationComponent>(selectedEntity);
                                }

                                // Giving some breathing space between each component in the entity properties panel
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(3);    // Sets the signature for AnimationComponent to be 1. Means the selected entity doesn't have.
                            }

                            if (entitySignature.test(4))    // BulletComponent
                            {
                                //ImGui::Spacing();
                                //ImGui::Separator();
                                //ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(4);
                            }

                            // Checks if there is TransformComponent. If yes, show its variables
                            if (entitySignature.test(5))    // TransformComponent
                            {
                                TransformComponent& transformComponent = ecsInterface.GetComponent<TransformComponent>(selectedEntity);

                                // Transform Component UI for selected entity
                                if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    // Translation
                                    glm::vec2 prevPosition = transformComponent.position;

                                    if (ImGui::InputFloat2("Translation", glm::value_ptr(transformComponent.position)))
                                    {
                                        if (prevPosition != transformComponent.position)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "TransformComponent", "position", transformComponent.position, prevPosition, transformComponent.position);
                                        }
                                    }

                                    // Rotation
                                    float prevRotation = transformComponent.rotation;

                                    if (ImGui::SliderFloat("Rotation", &transformComponent.rotation, 0.0f, 360.0f))
                                    {
                                        if (prevRotation != transformComponent.rotation)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "TransformComponent", "rotation", transformComponent.rotation, prevRotation, transformComponent.rotation);
                                        }
                                    }

                                    // Scale
                                    glm::vec2 prevScale = transformComponent.scale;

                                    if (ImGui::InputFloat2("Scale", glm::value_ptr(transformComponent.scale)))
                                    {
                                        if (prevScale != transformComponent.scale)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "TransformComponent", "scale", transformComponent.scale, prevScale, transformComponent.scale);
                                        }
                                    }

                                    // Button to toggle gizmo visibility
                                    bool prevShowGizmos = showGizmos;

                                    if (ImGui::Button(showGizmos ? "Scale & Rotate Unlocked" : "Scale & Rotate Locked", ImVec2(200, 30)))
                                    {
                                        showGizmos = !showGizmos;

                                        // Only push undo if you want to track gizmo state changes
                                        undoRedoManager.PushUndo(selectedEntity, "TransformComponent", "showGizmos", showGizmos, prevShowGizmos, showGizmos);
                                    }
                                }

                                // Remove component portion. Cannot do so because of the mSortedEntities loop at the top of this file
                                //if (ImGui::Button("Remove Component"))
                                //{
                                //    inverseSignature.flip(5);
                                //    inverseSignature.flip(6);
                                //    inverseSignature.flip(7);
                                //    inverseSignature.flip(8);
                                //    ecsInterface.RemoveComponent<TransformComponent>(selectedEntity);
                                //    ecsInterface.RemoveComponent<RenderComponent>(selectedEntity);
                                //    ecsInterface.RemoveComponent<LayerComponent>(selectedEntity);
                                //    ecsInterface.RemoveComponent<TextComponent>(selectedEntity);
                                //    
                                //    entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // Fetch updated signature
                                //}

                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(5);    // Sets the signature for TransformComponent to be 1. Means the selected entity doesn't have.
                            }

                            // Checks if there is RenderComponent. If yes, show its variables
                            if (entitySignature.test(6))    // RenderComponent
                            {
                                RenderComponent& renderComponent = ecsInterface.GetComponent<RenderComponent>(selectedEntity);

                                // Track initial values for undo
                                std::string prevTextureName = renderComponent.textureID;
                                glm::vec3 prevColor = renderComponent.color;
                                float prevAlpha = renderComponent.alpha;

                                if (ImGui::CollapsingHeader("Texture Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    // Show current texture
                                    std::string currentTextureName = renderComponent.textureID;         // Rendercomponent.textureID contains the path of the Texture
                                    ImGui::Text("Current Texture: %s", currentTextureName.c_str());     // Show current texture name aka path
                                    
                                    if (ImGui::BeginCombo("Change Texture", newTextureName.empty() ? "Select Texture" : newTextureName.c_str()))
                                    {
                                        for (const auto& texturePair : textureAssets)
                                        {
                                            const std::string& assetName = texturePair.first;       // Get the name of the texture
                                            bool isSelected = (newTextureName == assetName);        // Check if the texture is selected

                                            if (ImGui::Selectable(assetName.c_str(), isSelected))   // If the texture is selected in the combo box, set it as the new texture
                                            {
                                                newTextureName = assetName;                         // Update the selected texture name
                                            }
                                            if (isSelected)
                                            {
                                                ImGui::SetItemDefaultFocus();                       // Auto scroll to and focus on the selected item in the combo box
                                            }
                                        }
                                        ImGui::EndCombo();                                          // End combo box
                                    }

                                    // Apply texture change
                                    if (!newTextureName.empty() && ImGui::Button("Apply Texture"))  // Button to apply selected texture
                                    {
                                        // Check if the texture name has actually changed
                                        if (prevTextureName != newTextureName)  // Only push undo if the texture has changed
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "RenderComponent", "textureID", renderComponent.textureID, prevTextureName, newTextureName);
                                            renderComponent.textureID = newTextureName;
                                        }
                                    }

                                    // Color Edit and Alpha Slider
                                    ImGui::ColorEdit3("Color", glm::value_ptr(renderComponent.color));
                                    ImGui::SliderFloat("Alpha", &renderComponent.alpha, 0.0f, 1.0f);

                                    // Color and Alpha (if applicable)
                                    if (prevColor != renderComponent.color)  // Only push undo if color has changed
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "RenderComponent", "color", renderComponent.color, prevColor, renderComponent.color);
                                    }

                                    if (prevAlpha != renderComponent.alpha)  // Only push undo if alpha has changed
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "RenderComponent", "alpha", renderComponent.alpha, prevAlpha, renderComponent.alpha);
                                    }
                                    ImGui::Spacing();
                                    ImGui::Separator();
                                    ImGui::Spacing();
                                }
                            }
                            else
                            {
                                inverseSignature.set(6);
                            }
                                // Checks if there is LayerCompomnent. If yes, show its variables
                                if (entitySignature.test(7))    // LayerComponent
                                {
                                    if (ImGui::CollapsingHeader("Layer Component", ImGuiTreeNodeFlags_DefaultOpen))
                                    {
                                        LayerComponent& layerComponent = ecsInterface.GetComponent<LayerComponent>(selectedEntity);
                                        int prevSortID = layerComponent.sortID;                     // Track previous values for undo
                                        
                                        if (ImGui::InputInt("Sort ID", &layerComponent.sortID))     // Input for sortID
                                        {
                                            if (layerComponent.sortID != prevSortID)                // Check if sortID has actually changed
                                            {
                                                undoRedoManager.PushUndo(selectedEntity, "LayerComponent", "sortID", layerComponent.sortID, prevSortID, layerComponent.sortID);
                                            }
                                        }

                                        // Dropdown menu for layerID
                                        const char* layerNames[] = { "Background", "Character", "Foreground", "UI", "Debug" };
                                        int currentLayer = static_cast<int>(layerComponent.layerID);    // Convert Layer enum to integer

                                        if (ImGui::Combo("Layer", &currentLayer, layerNames, IM_ARRAYSIZE(layerNames)))
                                        {
                                            Layer newLayer = static_cast<Layer>(currentLayer);
                                            if (newLayer != layerComponent.layerID)                     // Check if layerID has actually changed
                                            {
                                                Layer prevLayerID = layerComponent.layerID;             // Capture the previous value before updating
                                                undoRedoManager.PushUndo(selectedEntity, "LayerComponent", "layerID", layerComponent.layerID, prevLayerID, newLayer);
                                                layerComponent.layerID = newLayer;                      // Update the layerID based on selection
                                            }
                                        }
                                        ImGui::Spacing();
                                        ImGui::Separator();
                                        ImGui::Spacing();
                                    }
                                }
                                else
                                {
                                    inverseSignature.set(7);
                                }

                                // Checks if there is TextComponent. If yes, show its variables
                                if (entitySignature.test(8)) // TextComponent
                                {
                                    if (ImGui::CollapsingHeader("Text Component", ImGuiTreeNodeFlags_DefaultOpen))
                                    {
                                        TextComponent& textComponent = ecsInterface.GetComponent<TextComponent>(selectedEntity);

                                        // Track previous values for undo
                                        std::string prevText = textComponent.text;
                                        float prevFontSize = textComponent.fontSize;
                                        glm::vec3 prevColor = textComponent.color;
                                        std::string prevFontName = textComponent.fontName;
                                        glm::vec2 prevOffset = textComponent.offset;

                                        // Input for text content
                                        char buffer[256];
                                        strncpy(buffer, textComponent.text.c_str(), sizeof(buffer));
                                        buffer[sizeof(buffer) - 1] = '\0'; // Ensure null termination

                                        if (ImGui::InputText("Text", buffer, sizeof(buffer)))
                                        {
                                            std::string newTextName = buffer;
                                            if (newTextName != prevText)  // Check if text has changed
                                            {
                                                textComponent.text = newTextName; // Update the component's text value
                                                undoRedoManager.PushUndo(selectedEntity, "TextComponent", "text", textComponent.text, prevText, textComponent.text);
                                            }
                                        }

                                        // Input for font size
                                        if (ImGui::InputFloat("Font Size", &textComponent.fontSize))
                                        {
                                            if (textComponent.fontSize != prevFontSize)  // Check if font size has changed
                                            {
                                                undoRedoManager.PushUndo(selectedEntity, "TextComponent", "fontSize", textComponent.fontSize, prevFontSize, textComponent.fontSize);
                                            }
                                        }

                                        // Separate color picker for text
                                        if (ImGui::ColorEdit3("Text Color", glm::value_ptr(textComponent.color)))
                                        {
                                            if (textComponent.color != prevColor)  // Check if color has changed
                                            {
                                                undoRedoManager.PushUndo(selectedEntity, "TextComponent", "color", textComponent.color, prevColor, textComponent.color);
                                            }
                                        }

                                        // Dropdown for font selection
                                        const char* availableFonts[] = { "Rubik", "Salmon", "Exo2-bold"}; // add here font names
                                        int currentFontIndex = -1;

                                        // Find the current font index
                                        for (int i = 0; i < IM_ARRAYSIZE(availableFonts); i++)
                                        {
                                            if (textComponent.fontName == availableFonts[i])
                                            {
                                                currentFontIndex = i;
                                                break;
                                            }
                                        }

                                        // Font selection dropdown
                                        if (ImGui::Combo("Font", &currentFontIndex, availableFonts, IM_ARRAYSIZE(availableFonts)))
                                        {
                                            // Create a std::string from the selected font
                                            std::string newFontName = availableFonts[currentFontIndex];

                                            // Only trigger the undo action if the font has actually changed
                                            if (textComponent.fontName != newFontName)
                                            {
                                                undoRedoManager.PushUndo(selectedEntity, "TextComponent", "fontName", textComponent.fontName, prevFontName, newFontName);
                                                textComponent.fontName = newFontName; // Update the selected font
                                            }
                                        }

                                        // Input for position offset
                                        if (ImGui::InputFloat2("Offset", glm::value_ptr(textComponent.offset)))
                                        {
                                            if (textComponent.offset != prevOffset)  // Check if offset has changed
                                            {
                                                undoRedoManager.PushUndo(selectedEntity, "TextComponent", "offset", textComponent.offset, prevOffset, textComponent.offset);
                                            }
                                        }                                        
                                        ImGui::Spacing();
                                        ImGui::Separator();
                                        ImGui::Spacing();

                                        if (ImGui::Button("Remove Text Component")) 
                                        {
                                            inverseSignature.flip(8);
                                            undoRedoManager.PushUndoComponent(selectedEntity, textComponent);
                                            ecsInterface.RemoveComponent<TextComponent>(selectedEntity);
                                            entitySignature = ecsInterface.GetEntitySignature(selectedEntity); 
                                        }
                                    }
                                }
                                else
                                {
                                    inverseSignature.set(8);
                                }

                            // Checks if there is PlayerComponent. If yes, show its variables
                            if (entitySignature.test(9))    // PlayerComponent
                            {
                                PlayerComponent& playerComponent = ecsInterface.GetComponent<PlayerComponent>(selectedEntity);

                                if (ImGui::CollapsingHeader("Player Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                }

                                if (ImGui::Button("Remove Player Component"))
                                {
                                    inverseSignature.flip(2);
                                    
                                    if (ecsInterface.HasComponent<MovementComponent>(selectedEntity)) {
                                        MovementComponent& movement = ecsInterface.GetComponent<MovementComponent>(selectedEntity);
                                        movement.baseVelocity.x = 0;
                                        movement.baseVelocity.y = 0;
                                    }
                                    undoRedoManager.PushUndoComponent(selectedEntity, playerComponent);
                                    ecsInterface.RemoveComponent<PlayerComponent>(selectedEntity);
                                    entitySignature = ecsInterface.GetEntitySignature(selectedEntity); // Fetch updated signature
                                    
                                }
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(9);
                            }

                            // Checks if there is ButtonCompomnent. If yes, show its variables
                            if (entitySignature.test(10))    // ButtonComponent
                            {
                                if (ImGui::CollapsingHeader("Button Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    ButtonComponent& buttonComponent = ecsInterface.GetComponent<ButtonComponent>(selectedEntity);
                                    std::vector<const char*> audioAssetNames;

                                    // Loops through audio assets to get the names of audio assets
                                    for (const auto& audioPair : audioAssets)
                                    {
                                        audioAssetNames.push_back(audioPair.first.c_str());
                                    }

                                    static int selectedPressedAudioIndex = 0; // Holds the current selection index

                                    for (size_t i = 0; i < audioAssetNames.size(); ++i)
                                    {
                                        if (buttonComponent.PressedAudio == audioAssetNames[i])
                                        {
                                            selectedPressedAudioIndex = static_cast<int>(i);
                                            break;
                                        }
                                    }

                                    static int selectedHoverAudioIndex = 0; // Holds the current selection index

                                    for (size_t i = 0; i < audioAssetNames.size(); ++i)
                                    {
                                        if (buttonComponent.HoverAudio == audioAssetNames[i])
                                        {
                                            selectedHoverAudioIndex = static_cast<int>(i);
                                            break;
                                        }
                                    }

                                    char labelBuffer[128];
                                    strncpy(labelBuffer, buttonComponent.label.c_str(), sizeof(labelBuffer));
                                    labelBuffer[sizeof(labelBuffer) - 1] = '\0'; // Ensure null-termination

                                    char idleTextureBuffer[128];
                                    strncpy(idleTextureBuffer, buttonComponent.idleTextureID.c_str(), sizeof(idleTextureBuffer));
                                    idleTextureBuffer[sizeof(idleTextureBuffer) - 1] = '\0';

                                    char hoverTextureBuffer[128];
                                    strncpy(hoverTextureBuffer, buttonComponent.hoverTextureID.c_str(), sizeof(hoverTextureBuffer));
                                    hoverTextureBuffer[sizeof(hoverTextureBuffer) - 1] = '\0';

                                    char pressedTextureBuffer[128];
                                    strncpy(pressedTextureBuffer, buttonComponent.pressedTextureID.c_str(), sizeof(pressedTextureBuffer));
                                    pressedTextureBuffer[sizeof(pressedTextureBuffer) - 1] = '\0';

                                    // Track previous values for undo
                                    std::string prevLabel = buttonComponent.label;
                                    std::string prevIdleTexture = buttonComponent.idleTextureID;
                                    std::string prevHoverTexture = buttonComponent.hoverTextureID;
                                    std::string prevPressedTexture = buttonComponent.pressedTextureID;
                                    std::string prevPressedAudio = buttonComponent.PressedAudio;
                                    std::string prevHoverAudio = buttonComponent.HoverAudio;
                                    std::string prevUpdateFunctionName = buttonComponent.UpdateFunctionName;
                                    ButtonState prevState = buttonComponent.state;
                                    float prevPressCooldown = buttonComponent.pressCooldown;
                                    float prevPressTimeRemaining = buttonComponent.pressTimeRemaining;

                                    // Inputs for text fields
                                    if (ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer)))
                                    {
                                        std::string newButtonLabel = labelBuffer;
                                        if (newButtonLabel != prevLabel) // Check if label has changed
                                        {
                                            buttonComponent.label = newButtonLabel;
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "label", buttonComponent.label, prevLabel, buttonComponent.label);
                                        }
                                    }
                                    if (ImGui::InputText("Idle Texture ID", idleTextureBuffer, sizeof(idleTextureBuffer)))
                                    {
                                        std::string newIdleTextureID = idleTextureBuffer;
                                        if (newIdleTextureID != prevIdleTexture) // Check if idle texture has changed
                                        {
                                            buttonComponent.idleTextureID = newIdleTextureID;
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "idleTextureID", buttonComponent.idleTextureID, prevIdleTexture, buttonComponent.idleTextureID);
                                        }
                                    }
                                    if (ImGui::InputText("Hover Texture ID", hoverTextureBuffer, sizeof(hoverTextureBuffer)))
                                    {
                                        std::string newHoverTexture = hoverTextureBuffer;
                                        if (newHoverTexture != prevHoverTexture) // Check if hover texture has changed
                                        {
                                            buttonComponent.hoverTextureID = newHoverTexture;
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "hoverTextureID", buttonComponent.hoverTextureID, prevHoverTexture, buttonComponent.hoverTextureID);
                                        }
                                    }
                                    if (ImGui::InputText("Pressed Texture ID", pressedTextureBuffer, sizeof(pressedTextureBuffer)))
                                    {
                                        std::string newPressedTexture = pressedTextureBuffer;
                                        if (newPressedTexture != prevPressedTexture) // Check if pressed texture has changed
                                        {
                                            buttonComponent.pressedTextureID = newPressedTexture;
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "pressedTextureID", buttonComponent.pressedTextureID, prevPressedTexture, buttonComponent.pressedTextureID);
                                        }
                                    }

                                    // Retrieve all registered button functions dynamically
                                    std::vector<std::string> updateFunctionOptions;
                                    for (const auto& pair : GlobalLogicManager.GetAllRegisteredButtonFunctions()) {
                                        updateFunctionOptions.push_back(pair.first); // Store function names
                                    }

                                    // Ensure selected index is valid
                                    static int selectedUpdateFunctionIndex = 0;
                                    for (size_t i = 0; i < updateFunctionOptions.size(); ++i) {
                                        if (buttonComponent.UpdateFunctionName == updateFunctionOptions[i]) {
                                            selectedUpdateFunctionIndex = static_cast<int>(i);
                                            break;
                                        }
                                    }

                                    if (ImGui::Combo("Update Function Name", &selectedUpdateFunctionIndex,
                                        [](void* userData, int idx, const char** out_text) -> bool {
                                            auto* items = static_cast<const std::vector<std::string>*>(userData);
                                            if (idx < 0 || idx >= static_cast<int>(items->size())) return false;
                                            *out_text = (*items)[idx].c_str();
                                            return true;
                                        },
                                        (void*)&updateFunctionOptions, static_cast<int>(updateFunctionOptions.size())))
                                    {
                                        if (selectedUpdateFunctionIndex >= 0 && selectedUpdateFunctionIndex < static_cast<int>(updateFunctionOptions.size()))
                                        {
                                            std::string newUpdateFunctionName = updateFunctionOptions[selectedUpdateFunctionIndex];

                                            // Check if the function name has changed
                                            if (newUpdateFunctionName != prevUpdateFunctionName)
                                            {
                                                // Push undo before updating the value
                                                undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "UpdateFunctionName", buttonComponent.UpdateFunctionName, prevUpdateFunctionName, newUpdateFunctionName);

                                                buttonComponent.UpdateFunctionName = newUpdateFunctionName;

                                                // Update previous function name for future comparison
                                                prevUpdateFunctionName = newUpdateFunctionName;

                                                // Initialize the button logic
                                                GlobalLogicManager.InitializeButton(selectedEntity);
                                            }
                                        }
                                    }

                                    // Combo Dropdown for Pressed Audio
                                    if (ImGui::Combo("Pressed Audio", &selectedPressedAudioIndex, audioAssetNames.data(), static_cast<int>(audioAssetNames.size())))
                                    {
                                        std::string newPressedAudio = audioAssetNames[selectedPressedAudioIndex];

                                        // Check if the audio asset has changed
                                        if (buttonComponent.PressedAudio != newPressedAudio)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "PressedAudio", buttonComponent.PressedAudio, prevPressedAudio, newPressedAudio);
                                        }

                                        // Update the button component's pressed audio
                                        buttonComponent.PressedAudio = newPressedAudio;
                                        prevPressedAudio = newPressedAudio; // Store the previous value for undo
                                    }

                                    // Combo Dropdown for Hover Audio
                                    if (ImGui::Combo("Hover Audio", &selectedHoverAudioIndex, audioAssetNames.data(), static_cast<int>(audioAssetNames.size())))
                                    {
                                        std::string newHoverAudio = audioAssetNames[selectedHoverAudioIndex];

                                        // Check if the hover audio has changed
                                        if (buttonComponent.HoverAudio != newHoverAudio)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "HoverAudio", buttonComponent.HoverAudio, prevHoverAudio, newHoverAudio);
                                        }

                                        // Update the button component's hover audio
                                        buttonComponent.HoverAudio = newHoverAudio;
                                        prevHoverAudio = newHoverAudio; // Store the previous value for undo
                                    }

                                    // State dropdown
                                    const char* buttonStates[] = { "Idle", "Hover", "Pressed" };
                                    int currentState = static_cast<int>(buttonComponent.state);
                                    if (ImGui::Combo("State", &currentState, buttonStates, IM_ARRAYSIZE(buttonStates)))
                                    {
                                        ButtonState newState = static_cast<ButtonState>(currentState);  // New state selected
                                        if (buttonComponent.state != newState)                          // Check if state has changed
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "state", buttonComponent.state, prevState, newState);
                                        }
                                        prevState = buttonComponent.state;                              // Store the previous value for undo
                                        buttonComponent.state = newState;                               // Update to the new state
                                    }

                                    // Press Cooldown and Remaining Time
                                    if (ImGui::InputFloat("Press Cooldown", &buttonComponent.pressCooldown))
                                    {
                                        if (buttonComponent.pressCooldown != prevPressCooldown) // Check if press cooldown has changed
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "pressCooldown", buttonComponent.pressCooldown, prevPressCooldown, buttonComponent.pressCooldown);
                                        }
                                    }

                                    if (ImGui::InputFloat("Press Time Remaining", &buttonComponent.pressTimeRemaining))
                                    {
                                        if (buttonComponent.pressTimeRemaining != prevPressTimeRemaining) // Check if press time remaining has changed
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "ButtonComponent", "pressTimeRemaining", buttonComponent.pressTimeRemaining, prevPressTimeRemaining, buttonComponent.pressTimeRemaining);
                                        }
                                    }
                                }
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(10);
                            }

                            // Checks if there is a TimelineComponent. If yes, show its variables
                            if (entitySignature.test(11))    // TimelineComponent
                            {
                                if (ImGui::CollapsingHeader("Timeline Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    TimelineComponent& timelineComponent = ecsInterface.GetComponent<TimelineComponent>(selectedEntity);

                                    // Store previous values for undo
                                    float prevInternalTimer = timelineComponent.InternalTimer;
                                    float prevTransitionDuration = timelineComponent.TransitionDuration;
                                    float prevTransitionInDelay = timelineComponent.TransitionInDelay;
                                    float prevTransitionOutDelay = timelineComponent.TransitionOutDelay;
                                    float prevDelayAccumulated = timelineComponent.DelayAccumulated;
                                    bool prevActive = timelineComponent.Active;
                                    bool prevIsTransitioningIn = timelineComponent.IsTransitioningIn;
                                    std::string prevTimelineTag = timelineComponent.TimelineTag;
                                    std::string prevTransitionInFunction = timelineComponent.TransitionInFunctionName;
                                    std::string prevTransitionOutFunction = timelineComponent.TransitionOutFunctionName;
                                    float prevStartPosition = timelineComponent.startPosition;
                                    float prevEndPosition = timelineComponent.endPosition;

                                    // Inputs for Animation Timing
                                    if (ImGui::InputFloat("Internal Timer", &timelineComponent.InternalTimer))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "InternalTimer", timelineComponent.InternalTimer, prevInternalTimer, timelineComponent.InternalTimer);
                                    }
                                    if (ImGui::InputFloat("Transition Duration", &timelineComponent.TransitionDuration))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TransitionDuration", timelineComponent.TransitionDuration, prevTransitionDuration, timelineComponent.TransitionDuration);
                                    }
                                    if (ImGui::InputFloat("Transition In Delay", &timelineComponent.TransitionInDelay))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TransitionInDelay", timelineComponent.TransitionInDelay, prevTransitionInDelay, timelineComponent.TransitionInDelay);
                                    }
                                    if (ImGui::InputFloat("Transition Out Delay", &timelineComponent.TransitionOutDelay))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TransitionOutDelay", timelineComponent.TransitionOutDelay, prevTransitionOutDelay, timelineComponent.TransitionOutDelay);
                                    }
                                    if (ImGui::InputFloat("Delay Accumulated", &timelineComponent.DelayAccumulated))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "DelayAccumulated", timelineComponent.DelayAccumulated, prevDelayAccumulated, timelineComponent.DelayAccumulated);
                                    }
                                    if (ImGui::Checkbox("Active", &timelineComponent.Active))       // Toggle Active & Transitioning In
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "Active", timelineComponent.Active, prevActive, timelineComponent.Active);
                                    }
                                    if (ImGui::Checkbox("Is Transitioning In", &timelineComponent.IsTransitioningIn))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "IsTransitioningIn", timelineComponent.IsTransitioningIn, prevIsTransitioningIn, timelineComponent.IsTransitioningIn);
                                    }

                                    // Timeline Tag Input
                                    char timelineTagBuffer[128];
                                    strncpy(timelineTagBuffer, timelineComponent.TimelineTag.c_str(), sizeof(timelineTagBuffer));
                                    timelineTagBuffer[sizeof(timelineTagBuffer) - 1] = '\0';
                                    if (ImGui::InputText("Timeline Tag", timelineTagBuffer, sizeof(timelineTagBuffer)))
                                    {
                                        std::string newTimelineTag = timelineTagBuffer;
                                        if (newTimelineTag != prevTimelineTag)
                                        {
                                            timelineComponent.TimelineTag = newTimelineTag;
                                            undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TimelineTag", timelineComponent.TimelineTag, prevTimelineTag, timelineComponent.TimelineTag);
                                        }
                                    }

                                    // Retrieve all registered timeline functions dynamically
                                    std::vector<std::string> transitionFunctionOptions;
                                    for (const auto& pair : GlobalLogicManager.GetAllRegisteredTimelineFunctions()) {
                                        transitionFunctionOptions.push_back(pair.first); // Store function names
                                    }

                                    // Ensure selected index is valid for Transition In
                                    static int selectedTransitionInIndex = 0;
                                    for (size_t i = 0; i < transitionFunctionOptions.size(); ++i) {
                                        if (timelineComponent.TransitionInFunctionName == transitionFunctionOptions[i]) {
                                            selectedTransitionInIndex = static_cast<int>(i);
                                            break;
                                        }
                                    }

                                    // Ensure selected index is valid for Transition Out
                                    static int selectedTransitionOutIndex = 0;
                                    for (size_t i = 0; i < transitionFunctionOptions.size(); ++i) {
                                        if (timelineComponent.TransitionOutFunctionName == transitionFunctionOptions[i]) {
                                            selectedTransitionOutIndex = static_cast<int>(i);
                                            break;
                                        }
                                    }

                                    // Dropdown for selecting Transition In Function
                                    if (ImGui::Combo("Transition In Function", &selectedTransitionInIndex,
                                        [](void* userData, int idx, const char** out_text) -> bool {
                                            auto* items = static_cast<const std::vector<std::string>*>(userData);
                                            if (idx < 0 || idx >= static_cast<int>(items->size())) return false;
                                            *out_text = (*items)[idx].c_str();
                                            return true;
                                        },
                                        (void*)&transitionFunctionOptions, static_cast<int>(transitionFunctionOptions.size())))
                                    {
                                        if (selectedTransitionInIndex >= 0 && selectedTransitionInIndex < static_cast<int>(transitionFunctionOptions.size()))
                                        {
                                            std::string newTransitionInFunction = transitionFunctionOptions[selectedTransitionInIndex];
                                            if (newTransitionInFunction != prevTransitionInFunction)
                                            {
                                                // Push undo before updating the value
                                                undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TransitionInFunctionName", timelineComponent.TransitionInFunctionName, prevTransitionInFunction, newTransitionInFunction);

                                                // Update the component
                                                timelineComponent.TransitionInFunctionName = newTransitionInFunction;

                                                // Reinitialize the timeline logic
                                                GlobalLogicManager.InitializeTimeline(selectedEntity);
                                            }
                                        }
                                    }

                                    // Dropdown for selecting Transition Out Function
                                    if (ImGui::Combo("Transition Out Function", &selectedTransitionOutIndex,
                                        [](void* userData, int idx, const char** out_text) -> bool {
                                            auto* items = static_cast<const std::vector<std::string>*>(userData);
                                            if (idx < 0 || idx >= static_cast<int>(items->size())) return false;
                                            *out_text = (*items)[idx].c_str();
                                            return true;
                                        },
                                        (void*)&transitionFunctionOptions, static_cast<int>(transitionFunctionOptions.size())))
                                    {
                                        if (selectedTransitionOutIndex >= 0 && selectedTransitionOutIndex < static_cast<int>(transitionFunctionOptions.size()))
                                        {
                                            std::string newTransitionOutFunction = transitionFunctionOptions[selectedTransitionOutIndex];
                                            if (newTransitionOutFunction != prevTransitionOutFunction)
                                            {
                                                // Push undo before updating the value
                                                undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "TransitionOutFunctionName", timelineComponent.TransitionOutFunctionName, prevTransitionOutFunction, newTransitionOutFunction);

                                                // Update the component
                                                timelineComponent.TransitionOutFunctionName = newTransitionOutFunction;

                                                // Reinitialize the timeline logic
                                                GlobalLogicManager.InitializeTimeline(selectedEntity);
                                            }
                                        }
                                    }


                                    // Position Inputs
                                    if (ImGui::InputFloat("Start Position", &timelineComponent.startPosition))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "startPosition", timelineComponent.startPosition, prevStartPosition, timelineComponent.startPosition);
                                    }
                                    if (ImGui::InputFloat("End Position", &timelineComponent.endPosition))
                                    {
                                        undoRedoManager.PushUndo(selectedEntity, "TimelineComponent", "endPosition", timelineComponent.endPosition, prevEndPosition, timelineComponent.endPosition);
                                    }
                                }
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();
                            }
                            else
                            {
                                inverseSignature.set(11);
                            }

                            // Checks if there is ParticleComponent. If yes, show its variables
                            if (entitySignature.test(12))    // ParticleComponent
                            {
                                if (ImGui::CollapsingHeader("Particle Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    if (ecsInterface.HasComponent<ParticleComponent>(selectedEntity))
                                    {
                                        ParticleComponent& particleData = ecsInterface.GetComponent<ParticleComponent>(selectedEntity);

                                        if (ecsInterface.HasComponent<TransformComponent>(selectedEntity))
                                        {
                                            TransformComponent& positionComponent = ecsInterface.GetComponent<TransformComponent>(selectedEntity);
                                        
                                            particleData.position = positionComponent.position;

                                            // Show current texture for Particle
                                            ImGui::Text("Current Particle Texture: %s", particleData.textureName.c_str());  // Display current texture name

                                            // Start ImGui combo box for texture selection
                                            if (ImGui::BeginCombo("Particle Texture", newParticleTextureName.empty() ? "Select Texture" : newParticleTextureName.c_str()))
                                            {
                                                for (const auto& texturePair : textureAssets)
                                                {
                                                    const std::string& assetName = texturePair.second.name;  // Get texture name
                                                    bool isSelected = (newParticleTextureName == assetName);  // Check if selected texture matches current

                                                    if (ImGui::Selectable(assetName.c_str(), isSelected))  // If selected, update texture name
                                                    {
                                                        // Push Undo action before changing the texture name
                                                        if (newParticleTextureName != assetName)
                                                        {
                                                            undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "textureName", particleData.textureName, particleData.textureName, assetName);
                                                            newParticleTextureName = assetName;  // Set the new selected texture name
                                                        }
                                                    }
                                                    if (isSelected)
                                                    {
                                                        ImGui::SetItemDefaultFocus();  // Auto focus on the selected item
                                                    }
                                                }
                                                ImGui::EndCombo();
                                            }

                                            // Apply texture change
                                            if (!newParticleTextureName.empty() && ImGui::Button("Apply Particle Texture"))
                                            {
                                                // Push Undo action before applying texture change
                                                if (particleData.textureName != newParticleTextureName)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "textureName", particleData.textureName, particleData.textureName, newParticleTextureName);
                                                    particleData.textureName = newParticleTextureName;  // Set the new texture name
                                                }
                                            }

                                            // Checkbox for active status
                                            bool prevActive = particleData.active;
                                            if (ImGui::Checkbox("Particle Active", &particleData.active))
                                            {
                                                // Push Undo action for active status change
                                                if (prevActive != particleData.active)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "active", particleData.active, prevActive, particleData.active);
                                                }
                                            }

                                            // Drag for position
                                            glm::vec2 prevPosition = particleData.position;
                                            if (ImGui::DragFloat2("Position", glm::value_ptr(particleData.position), 0.1f))
                                            {
                                                // Push Undo action for position change
                                                if (prevPosition != particleData.position)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "position", particleData.position, prevPosition, particleData.position);
                                                }
                                            }

                                            // Drag for velocity
                                            glm::vec2 prevVelocity = particleData.velocity;
                                            if (ImGui::DragFloat2("Velocity", glm::value_ptr(particleData.velocity), 0.1f))
                                            {
                                                // Push Undo action for velocity change
                                                if (prevVelocity != particleData.velocity)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "velocity", particleData.velocity, prevVelocity, particleData.velocity);
                                                }
                                            }

                                            // Color picker for color
                                            glm::vec3 prevColor = particleData.color;
                                            if (ImGui::ColorEdit3("Particle Color", glm::value_ptr(particleData.color)))
                                            {
                                                // Push Undo action for color change
                                                if (prevColor != particleData.color)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "color", particleData.color, prevColor, particleData.color);
                                                }
                                            }

                                            // Drag for size
                                            float prevSize = particleData.size;
                                            if (ImGui::DragFloat("Size", &particleData.size, 0.1f, 1.0f, 50.0f))
                                            {
                                                // Push Undo action for size change
                                                if (prevSize != particleData.size)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "size", particleData.size, prevSize, particleData.size);
                                                }
                                            }

                                            // Drag for lifetime
                                            float prevLifetime = particleData.life;
                                            if (ImGui::DragFloat("Lifetime", &particleData.life, 0.1f, 0.1f, 10.0f))
                                            {
                                                // Push Undo action for lifetime change
                                                if (prevLifetime != particleData.life)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "lifetime", particleData.life, prevLifetime, particleData.life);
                                                }
                                            }

                                            // Drag for emission rate
                                            float prevEmissionRate = particleData.emissionRate;
                                            if (ImGui::DragFloat("Emission Rate", &particleData.emissionRate, 1.0f, 1.0f, 100.0f))
                                            {
                                                // Push Undo action for emission rate change
                                                if (prevEmissionRate != particleData.emissionRate)
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "emissionRate", particleData.emissionRate, prevEmissionRate, particleData.emissionRate);
                                                }
                                            }

                                            // Dropdown menu for emission shape
                                            const char* emissionShapes[] = { "Circle", "Box", "ELLIPSE", "LINE", "SPIRAL", "RADIAL", "RANDOM", "WAVE", "CONE", "EXPLOSION" };
                                            EmissionShape prevShape = particleData.shape;  // Store the previous shape
                                            int shapeIndex = static_cast<int>(particleData.shape);

                                            if (ImGui::Combo("Emission Shape", &shapeIndex, emissionShapes, IM_ARRAYSIZE(emissionShapes)))
                                            {
                                                // Only push to Undo stack if the shape actually changed
                                                if (shapeIndex != static_cast<int>(prevShape))
                                                {
                                                    undoRedoManager.PushUndo(selectedEntity, "ParticleComponent", "shape", particleData.shape, prevShape, static_cast<EmissionShape>(shapeIndex));
                                                    particleData.shape = static_cast<EmissionShape>(shapeIndex); // Update the emission shape
                                                }
                                            }
                                        }
                                    }
                                    ImGui::Spacing();
                                    ImGui::Separator();
                                    ImGui::Spacing();
                                }
                            }
                            else
                            {
                                inverseSignature.set(12); // If no particle component, mark it for future additions
                            }

                            if (entitySignature.test(13))    // SpawnerComponent
                            {
                                if (ImGui::CollapsingHeader("Spawner Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    if (ecsInterface.HasComponent<SpawnerComponent>(selectedEntity))
                                    {
                                        SpawnerComponent& spawnerComponent = ecsInterface.GetComponent<SpawnerComponent>(selectedEntity);
                                        
                                        float prevSpawnInterval = spawnerComponent.spawnInterval;
                                        ImGui::InputFloat("Spawn Interval", &spawnerComponent.spawnInterval); // Allow editors to freely change the interval at which enemies spawn
                                        if (prevSpawnInterval != spawnerComponent.spawnInterval)
                                        {
                                            undoRedoManager.PushUndo(selectedEntity, "SpawnerComponent", "Spawn Interval", spawnerComponent.spawnInterval, prevSpawnInterval, spawnerComponent.spawnInterval);
                                        }

                                        if (ImGui::Button("Remove Spawner Component"))
                                        {
                                            inverseSignature.flip(13);
                                            undoRedoManager.PushUndoComponent(selectedEntity, spawnerComponent);
                                            ecsInterface.RemoveComponent<SpawnerComponent>(selectedEntity);
                                            entitySignature = ecsInterface.GetEntitySignature(selectedEntity);
                                        }
                                    }
                                    ImGui::Spacing();
                                    ImGui::Separator();
                                    ImGui::Spacing();
                                }
                            }
                            else
                            {
                                inverseSignature.set(13); // If no spawner component, allow it to be added by inversing the signature
                            }
                            if (entitySignature.test(14)) // UIBarComponent
                            {
                                if (ImGui::CollapsingHeader("UI Bar Component", ImGuiTreeNodeFlags_DefaultOpen))
                                {
                                    if (ecsInterface.HasComponent<UIBarComponent>(selectedEntity))
                                    {
                                        UIBarComponent& barComponent = ecsInterface.GetComponent<UIBarComponent>(selectedEntity);

                                        // Track previous values for undo
                                        float prevFillPercentage = barComponent.FillPercentage;
                                        glm::vec2 prevOffset = barComponent.offset;
                                        glm::vec2 prevScale = barComponent.scale;
                                        glm::vec2 prevFillOffset = barComponent.fillOffset;
                                        glm::vec2 prevFillSize = barComponent.fillSize;
                                        std::string prevFillTex = barComponent.fillTextureID;
                                        std::string prevBackingTex = barComponent.backingTextureID;

                                        // === Fill Percentage ===
                                        if (ImGui::SliderFloat("Fill Percentage", &barComponent.FillPercentage, 0.0f, 1.0f)) {
                                            if (barComponent.FillPercentage != prevFillPercentage)
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "FillPercentage", barComponent.FillPercentage, prevFillPercentage, barComponent.FillPercentage);
                                        }

                                        // === Offset ===
                                        if (ImGui::InputFloat2("Offset", &barComponent.offset.x)) {
                                            if (barComponent.offset != prevOffset)
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "Offset", barComponent.offset, prevOffset, barComponent.offset);
                                        }

                                        // === Scale ===
                                        if (ImGui::InputFloat2("Scale", &barComponent.scale.x)) {
                                            if (barComponent.scale != prevScale)
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "Scale", barComponent.scale, prevScale, barComponent.scale);
                                        }

                                        // === Fill Offset ===
                                        if (ImGui::InputFloat2("Fill Offset", &barComponent.fillOffset.x)) {
                                            if (barComponent.fillOffset != prevFillOffset)
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "fillOffset", barComponent.fillOffset, prevFillOffset, barComponent.fillOffset);
                                        }

                                        // === Fill Size ===
                                        if (ImGui::InputFloat2("Fill Size", &barComponent.fillSize.x)) {
                                            if (barComponent.fillSize != prevFillSize)
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "fillSize", barComponent.fillSize, prevFillSize, barComponent.fillSize);
                                        }

                                        // === Fill Color (vec3) ===
                                        ImGui::ColorEdit3("Fill Color (RGB)", &barComponent.fillColor.r);
                                        ImGui::SliderFloat("Fill Alpha", &barComponent.fillAlpha, 0.0f, 1.0f);

                                        // === Background Color (vec3) ===
                                        ImGui::ColorEdit3("Background Color (RGB)", &barComponent.bgColor.r);
                                        ImGui::SliderFloat("Background Alpha", &barComponent.bgAlpha, 0.0f, 1.0f);

                                        // === Fill Texture ID ===
                                        char fillTextureBuffer[128];
                                        strncpy(fillTextureBuffer, barComponent.fillTextureID.c_str(), sizeof(fillTextureBuffer));
                                        fillTextureBuffer[sizeof(fillTextureBuffer) - 1] = '\0';
                                        if (ImGui::InputText("Fill Texture ID", fillTextureBuffer, sizeof(fillTextureBuffer))) {
                                            std::string newFillTex = fillTextureBuffer;
                                            if (newFillTex != prevFillTex) {
                                                barComponent.fillTextureID = newFillTex;
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "fillTextureID", barComponent.fillTextureID, prevFillTex, newFillTex);
                                            }
                                        }

                                        // === Backing Texture ID ===
                                        char backingTextureBuffer[128];
                                        strncpy(backingTextureBuffer, barComponent.backingTextureID.c_str(), sizeof(backingTextureBuffer));
                                        backingTextureBuffer[sizeof(backingTextureBuffer) - 1] = '\0';
                                        if (ImGui::InputText("Backing Texture ID", backingTextureBuffer, sizeof(backingTextureBuffer))) {
                                            std::string newBackingTex = backingTextureBuffer;
                                            if (newBackingTex != prevBackingTex) {
                                                barComponent.backingTextureID = newBackingTex;
                                                undoRedoManager.PushUndo(selectedEntity, "UIBarComponent", "backingTextureID", barComponent.backingTextureID, prevBackingTex, newBackingTex);
                                            }
                                        }

                                        // === Remove UIBarComponent ===
                                        if (ImGui::Button("Remove UI Bar Component")) {
                                            inverseSignature.flip(14);
                                            undoRedoManager.PushUndoComponent(selectedEntity, barComponent);
                                            ecsInterface.RemoveComponent<UIBarComponent>(selectedEntity);
                                            entitySignature = ecsInterface.GetEntitySignature(selectedEntity);
                                        }
                                    }

                                    ImGui::Spacing();
                                    ImGui::Separator();
                                    ImGui::Spacing();
                                }
                            }
                            else
                            {
                                inverseSignature.set(14); // Allow UIBarComponent to be added when missing
                            }



                            if (inverseSignature.test(0) && ImGui::Button("Add Movement Component"))
                            {
                                ecsInterface.AddComponent<MovementComponent>(selectedEntity, MovementComponent{});
                            }

                            // Commenting this out here because the one below adds enemy already
                            if (inverseSignature.test(1) && ImGui::Button("Add Enemy Component"))
                            {
                                ecsInterface.AddComponent<EnemyComponent>(selectedEntity, EnemyComponent{});
                            }
                            if (inverseSignature.test(2) && ImGui::Button("Add Collision Component"))
                            {
                                ecsInterface.AddComponent<CollisionComponent>(selectedEntity, CollisionComponent{});
                            }
                            //if (inverseSignature.test(2) && ImGui::Button("Add Enemy Component"))
                            //{
                            //    ecsInterface.AddComponent<CollisionComponent>(selectedEntity, CollisionComponent{});
                            //    ecsInterface.AddComponent<MovementComponent>(selectedEntity, MovementComponent{});
                            //    ecsInterface.AddComponent<EnemyComponent>(selectedEntity, EnemyComponent{});
                            //    ecsInterface.AddComponent<BulletComponent>(selectedEntity, BulletComponent{});
                            //    inverseSignature.reset(2); // Update after adding the component
                            //}

                            if (inverseSignature.test(3) && ImGui::Button("Add Animation Component"))
                            {
                                std::cout << "Add Animation Component button pressed" << std::endl;
                                ecsInterface.AddComponent<AnimationComponent>(selectedEntity, AnimationComponent{});
                               inverseSignature.reset(3); // Update after adding the component
                            }

                            /*if (inverseSignature.test(4) && ImGui::Button("Add 
                            Component"))
                            {
                                ecsInterface.AddComponent<BulletComponent>(selectedEntity, BulletComponent{});
                            }*/

                            if (inverseSignature.test(5) && ImGui::Button("Add Transform Component"))
                            {
                                ecsInterface.AddComponent<TransformComponent>(selectedEntity, TransformComponent{});
                            }

                            if (inverseSignature.test(6) && ImGui::Button("Add Render Component"))
                            {
                                ecsInterface.AddComponent<RenderComponent>(selectedEntity, RenderComponent{});
                            }

                            if (inverseSignature.test(7) && ImGui::Button("Add Layer Component"))
                            {
                                ecsInterface.AddComponent<LayerComponent>(selectedEntity, LayerComponent{});
                            }

                            if (inverseSignature.test(8) && ImGui::Button("Add Text Component"))
                            {
                                ecsInterface.AddComponent<TextComponent>(selectedEntity, TextComponent{});
                            }

                            if (inverseSignature.test(9) && ImGui::Button("Add Player Component"))
                            {
                                PlayerComponent player;
                                player.health = 3.0;
                                ecsInterface.AddComponent<PlayerComponent>(selectedEntity, player);
                                if (ecsInterface.HasComponent<MovementComponent>(selectedEntity)) {
                                    MovementComponent& movement = ecsInterface.GetComponent<MovementComponent>(selectedEntity);
                                    movement.baseVelocity.x = 200;
                                    movement.baseVelocity.y = 200;
                                }
                            }

                            if (inverseSignature.test(10) && ImGui::Button("Add Button Component"))
                            {
                                ecsInterface.AddComponent<ButtonComponent>(selectedEntity, ButtonComponent{});
                            }

                            if (inverseSignature.test(11) && ImGui::Button("Add Timeline Component"))
                            {
                                ecsInterface.AddComponent<TimelineComponent>(selectedEntity, TimelineComponent{});
                            }

                            if (inverseSignature.test(12) && ImGui::Button("Add Particle Component"))
                            {
                                ecsInterface.AddComponent<ParticleComponent>(selectedEntity, ParticleComponent{});
                            }

                            if (inverseSignature.test(13) && ImGui::Button("Add Spawner Component"))
                            {
                                ecsInterface.AddComponent<SpawnerComponent>(selectedEntity, SpawnerComponent{});
                            }

                            /*if (ImGui::Button("Clone Entity", ImVec2(180, 30)))
                            {
                                ecsInterface.CloneEntity(selectedEntity);
                            }*/

                            // Calculate the Y-position to place the button at the bottom
                            float buttonHeight = ImGui::GetFrameHeightWithSpacing();
                            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - buttonHeight - ImGui::GetStyle().WindowPadding.y);

                        }

                        //// Close button
                        //if (ImGui::Button("Close", ImVec2(180, 30)))
                        //{
                        //    isPropertiesWindowOpen = false;  // Close properties window
                        //}
                        ImGui::End();
                    }

                    // Reset selectedEntity if the properties window is closed
                    if (!isPropertiesWindowOpen)
                    {
                        selectedEntity = std::numeric_limits<Entity>::max();
                    }
                }
            }
            ImGui::End();

            // Begin the DebugSystem ImGui window
            if (ImGui::Begin("DebugSystem", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                // Retrieve the DebugSystem instance
                Framework::DebugSystem* debugSystem = Framework::DebugSystem::GetInstance();

                // Get the cached percentage values
                std::unordered_map<std::string, double> systemPercentages = debugSystem->GetSystemUpdateTimes();

                // Prepare data for histogram
                std::vector<float> values;
                std::vector<const char*> labels;

                for (const auto& [systemName, percentage] : systemPercentages) {
                    values.push_back(static_cast<float>(percentage)); // Add percentage as float for the histogram
                    labels.push_back(systemName.c_str()); // Store the label for each system
                }

                // Display each system's update percentage as text
                for (size_t i = 0; i < values.size(); ++i) {
                    ImGui::Text("%s: %.2f%%", labels[i], values[i]);
                }

                // Plot the histogram for system performance percentages
                if (!values.empty()) {
                    ImGui::Text("System Performance Histogram");
                    ImGui::PlotHistogram("##SystemUsage", values.data(), static_cast<int>(values.size()), 0, nullptr, 0.0f, 100.0f, ImVec2(0, 100));
                }
            }
            // End the DebugSystem ImGui window
            ImGui::End();

            // Open a new window for game state controls (e.g., "Game Controls")
            if (ImGui::Begin("Game Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav))
            {
                // Get the width of the available region
                float windowWidth = ImGui::GetContentRegionAvail().x;

                // Set button size (adjust values as desired)
                ImVec2 buttonSize(50, 30); // Set the width and height of the button

                // Calculate the X position to center the buttons horizontally
                float centerX = (windowWidth - (3 * buttonSize.x + 2 * ImGui::GetStyle().ItemSpacing.x)) * 0.5f;

                // Add spacing on the left to center-align the buttons
                ImGui::SetCursorPosX(centerX);

                // Play Button with custom color
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));       // Light blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // Medium blue when hovered
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));  // Darker blue when active
                if (ImGui::Button("Play", buttonSize))
                {
                    // Set engine state to play mode
                    GlobalEntityAsset.SerializeEntities("Assets/Scene/EditorInstance.json");
                    engineState.SetPlay(true);
                }
                ImGui::PopStyleColor(3); // Restore color
                ImGui::SameLine(); // Place the next button on the same line

                // Pause Button with custom color
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));       // Light blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // Medium blue when hovered
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));  // Darker blue when active
                if (ImGui::Button("Pause", buttonSize))
                {
                    // Toggle pause state while in play mode
                    if (engineState.IsPlay())
                    {
                        engineState.SetPaused(!engineState.IsPaused());
                        GlobalAudio.UE_PauseAllAudio();
                    }
                    else
                    {
                        GlobalAudio.UE_ResumeAllAudio();
                    }
                }
                ImGui::PopStyleColor(3); // Restore color
                ImGui::SameLine(); // Place the next button on the same line

                // Stop Button with custom color
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));       // Red
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.0f, 0.0f, 1.0f)); // Darker red when hovered
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));  // Even darker red when active
                if (ImGui::Button("Stop", buttonSize))
                {
                    // Stop the game by setting play to false
                    engineState.SetPlay(false);
                    ecsInterface.ClearEntities(); // Clear all entity and load json to reset scene

                    GlobalAssetManager.UE_LoadEntities(filePath); // temporarily load this scene // cant load 
                }

                ImGui::PopStyleColor(3); // Restore color
                ImGui::SameLine(); // Place the next button on the same line

                // Debug Button with custom color
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));       // Light blue
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.9f, 1.0f)); // Medium blue when hovered
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));  // Darker blue when active
                if (ImGui::Button("Debug", buttonSize))
                {
                    // Toggle debug state while in play mode
                    engineState.SetInDebugMode(!engineState.IsInDebugMode());
                }
                ImGui::PopStyleColor(3); // Restore color
            }
            ImGui::End();

            if (ImGui::Begin("Main Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav))
            {
                ImVec2 viewportSize = ImGui::GetContentRegionAvail();
                float aspectRatio = static_cast<float>(projWidth) / static_cast<float>(projHeight);

                float newWidth = viewportSize.x;
                float newHeight = viewportSize.x / aspectRatio;

                newHeight = viewportSize.y;
                newWidth = viewportSize.y * aspectRatio;

                ImVec2 offset((viewportSize.x - newWidth) * 0.5f, (viewportSize.y - newHeight) * 0.5f);
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 absoluteOffset(windowPos.x + offset.x, windowPos.y + offset.y);

                Graphics::viewportOffsetX = absoluteOffset.x;
                Graphics::viewportOffsetY = absoluteOffset.y;
                Graphics::viewportWidth = newWidth;
                Graphics::viewportHeight = newHeight;

                ImGui::SetCursorPos(offset);
                ImGui::Image((ImTextureID)(intptr_t)gameTexture, ImVec2(newWidth, newHeight), ImVec2(0, 1), ImVec2(1, 0));

                ImVec2 viewportMin = absoluteOffset;
                ImVec2 viewportMax = ImVec2(absoluteOffset.x + newWidth, absoluteOffset.y + newHeight);

                for (Entity entity : ecsInterface.GetEntities())
                {
                    TransformComponent& transformComponent = ecsInterface.GetComponent<TransformComponent>(entity);

                    ImVec2 screenMin
                    (
                        absoluteOffset.x + (transformComponent.position.x - transformComponent.scale.x * 0.5f) * (newWidth / projWidth),
                        absoluteOffset.y + (transformComponent.position.y - transformComponent.scale.y * 0.5f) * (newHeight / projHeight)
                    );

                    ImVec2 screenMax
                    (
                        absoluteOffset.x + (transformComponent.position.x + transformComponent.scale.x * 0.5f) * (newWidth / projWidth),
                        absoluteOffset.y + (transformComponent.position.y + transformComponent.scale.y * 0.5f) * (newHeight / projHeight)
                    );

                    ImGui::SetCursorScreenPos(screenMin);
                    ImVec2 buttonSize(screenMax.x - screenMin.x, screenMax.y - screenMin.y);
                    std::string buttonID = "entity##" + std::to_string(entity);
                    if (ImGui::InvisibleButton(buttonID.c_str(), buttonSize))
                    {
                        selectedEntity = entity;
                        isPropertiesWindowOpen = true;
                    }

                    // Track the starting drag position when the drag starts
                    static ImVec2 dragStartPos;  // Store the mouse position when drag starts
                    static glm::vec2 entityStartPos;  // Store the entity position when drag starts

                    if (selectedEntity == entity && !engineState.IsPlay())
                    {
                        // Track interaction states
                        static bool isScaling = false;
                        static bool isRotating = false;

                        // Handle entity transformation UI next to the entity
                        ImVec2 entityGizmoPos(screenMax.x + 10, screenMin.y); // Position UI slightly to the right

                        if (showGizmos == true)
                        {
                            // **Scaling Gizmo (Next to the entity)**
                            ImGui::SetCursorScreenPos(entityGizmoPos);
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(100, 0, 0, 1));
                            ImGui::Text("Scale X/Y:");
                            ImGui::PushItemWidth(120.0f);
                            ImGui::SetCursorScreenPos(ImVec2(entityGizmoPos.x, entityGizmoPos.y + 20));
                            ImGui::PushID("ScaleGizmo");  // Unique ID to avoid conflicts
                            isScaling = ImGui::DragFloat2("##Scale", &transformComponent.scale.x, 0.5f, 0.1f, 2000.0f, "%.2f");
                            ImGui::PopID();
                            ImGui::PopItemWidth();

                            // **Rotation Gizmo (Below the Scale UI)**
                            ImVec2 rotationGizmoPos(entityGizmoPos.x, entityGizmoPos.y + 50); // Offset below scale UI
                            ImGui::SetCursorScreenPos(rotationGizmoPos);
                            ImGui::Text("Rotation:");
                            ImGui::PushItemWidth(120.0f);
                            ImGui::SetCursorScreenPos(ImVec2(rotationGizmoPos.x, rotationGizmoPos.y + 20));

                            ImGui::PushID("RotationGizmo");  // Unique ID for rotation gizmo
                            isRotating = ImGui::DragFloat("##Rotation", &transformComponent.rotation, 0.5f, -720.0f, 720.0f, "%.1f deg");
                            ImGui::PopID();
                            ImGui::PopItemWidth();
                            // Reset to default color after the text
                            ImGui::PopStyleColor();
                        }

                        // **Translation Logic** (if not scaling or rotating)
                        if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0) && !Graphics::IsMouseOutsideViewport(viewportMin, viewportMax) && showGizmos == false)
                        {
                            // Optional: Debug Bounding Boxes
                            auto* drawList = ImGui::GetWindowDrawList();
                            drawList->AddRect(screenMin, screenMax, IM_COL32(70, 200, 70, 255)); // Green bounding box for debugging

                            // Capture the starting mouse position and entity position at the beginning of the drag
                            ImVec2 mousePos = ImGui::GetMousePos();
                            dragStartPos = mousePos;
                            entityStartPos = transformComponent.position;
                        }

                        if (ImGui::IsMouseDragging(0) && !Graphics::IsMouseOutsideViewport(viewportMin, viewportMax) && showGizmos == false)
                        {
                            // Optional: Debug Bounding Boxes
                            auto* drawList = ImGui::GetWindowDrawList();
                            drawList->AddRect(screenMin, screenMax, IM_COL32(70, 200, 70, 255)); // Green bounding box for debugging

                            // Get the current mouse position
                            ImVec2 mousePos = ImGui::GetMousePos();

                            // Calculate the mouse delta (difference from the start position)
                            ImVec2 mouseDelta = ImVec2(mousePos.x - dragStartPos.x, mousePos.y - dragStartPos.y);

                            // Convert from screen space to world space based on the current scale
                            float dragX = (mouseDelta.x / newWidth) * projWidth;
                            float dragY = (mouseDelta.y / newHeight) * projHeight;

                            // Update the entity's position based on the drag
                            transformComponent.position.x = entityStartPos.x + dragX;
                            transformComponent.position.y = entityStartPos.y + dragY;
                        }
                    }
                }
            }
            ImGui::End();

            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Handle multiple viewports (for multi-window applications)
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
            glfwSetDropCallback(backup_current_context, DropCallback);

            if (ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::GetIO().KeyCtrl)      // Handle key press for Undo (Ctrl+Z)
            {
                if (undoRedoManager.CanUndo())
                {
                    undoRedoManager.Undo(); // Perform undo action
                    std::cout << "After Undo:" << std::endl;
                    undoRedoManager.PrintStackDetails(); // Print undo stack details
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Y) && ImGui::GetIO().KeyCtrl)      // Handle key press for Redo (Ctrl+Y)
            {
                if (undoRedoManager.CanRedo())
                {
                    undoRedoManager.Redo(); // Perform redo action
                    std::cout << "After Redo:" << std::endl;
                    undoRedoManager.PrintStackDetails(); // Print redo stack details
                }
            }
        }
    }

    // Get the name of the system
    std::string Graphics::GetName()
    {
        return "Graphics";
    }

    /* @ SETTING BACKGROUND COLOR */
    void Graphics::SetBackgroundColor(int r, int g, int b, GLclampf alpha)
    {
        // Normalize the RGB values from 0-255 to 0-1
        GLclampf normR = r / 255.0f;
        GLclampf normG = g / 255.0f;
        GLclampf normB = b / 255.0f;

        // Set the clear color
        glClearColor(normR, normG, normB, alpha);
    }

    /* @ Model Drawing Function @ */
    // -------------------------- //
    void Graphics::Model::draw()
    {
        //start shader program
        shdr_pgm.Use();
        glBindVertexArray(vaoid);

        // Checking if texture is being used to draw
        bool useTexture = (textureID != 0);

        // Binding the texture if its actually textured
        if (useTexture)
        {
            glBindTexture(GL_TEXTURE_2D, textureID);
        }

        // Set the texture usage flag in the shader
        GLuint useTextureLocation = glGetUniformLocation(shdr_pgm.GetHandle(), "useTexture");
        glUniform1i(useTextureLocation, useTexture);

        // Set the color uniform in the shader
        GLuint colorLocation = glGetUniformLocation(shdr_pgm.GetHandle(), "uColor");
        GLint alphaLocation = glGetUniformLocation(shdr_pgm.GetHandle(), "uAlpha");


        // Setting color and alpha
        glUniform3f(colorLocation, color.r, color.g, color.b);
        glUniform1f(alphaLocation, alpha);

        //for transparent background while loading PNG images
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //clamping of textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Setting matrix transform for Vertex File
        glUniformMatrix4fv(glGetUniformLocation(shdr_pgm.GetHandle(), "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shdr_pgm.GetHandle(), "viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shdr_pgm.GetHandle(), "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        //for changing of primitive_type when drawing objects;
        switch (primitive_type)
        {
        case GL_POINTS:
            glPointSize(10.f);
            glDrawArrays(primitive_type, 0, draw_cnt);
            break;

        case GL_LINES:
            glLineWidth(3.f);
            glDrawArrays(primitive_type, 0, draw_cnt);
            glLineWidth(1.f);
            break;

        case GL_TRIANGLE_FAN:
            glDrawArrays(primitive_type, 0, draw_cnt);
            break;

        case GL_TRIANGLES:
            glDrawElements(primitive_type, draw_cnt, GL_UNSIGNED_INT, 0);
            break;
        }

        glBindVertexArray(0);
        shdr_pgm.UnUse();
    }

    // Point Model //
    Graphics::Model Graphics::points_model(const glm::vec2& coordinate, const glm::vec3& clr_vtx)
    {
        // Use the coordinate directly (no conversion to NDC)
        glm::vec2 pos_vtx = coordinate;

        Graphics::Model mdl;

        glCreateBuffers(1, &mdl.vbo_hdl);
        glNamedBufferStorage(mdl.vbo_hdl, sizeof(glm::vec2), &pos_vtx, GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &mdl.vaoid);
        glEnableVertexArrayAttrib(mdl.vaoid, 0);
        glVertexArrayVertexBuffer(mdl.vaoid, 0, mdl.vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 0, 0);
        glBindVertexArray(0);

        mdl.primitive_type = GL_POINTS;
        mdl.setup_shdrpgm(UE_vs, UE_fs);
        mdl.color = clr_vtx;
        mdl.draw_cnt = 1; // Only one point
        mdl.primitive_cnt = 1; // Only one point
        mdl.alpha = 1.0f;

        // **Initialize the transformation matrices** (NEW)
        mdl.modelMatrix = glm::mat4(1.0f); // Identity matrix for the model
        mdl.viewMatrix = glm::mat4(1.0f);  // Identity matrix for the view (for 2D)
        mdl.projectionMatrix = glm::ortho(0.0f, projWidth, projHeight, 0.0f);

        return mdl;
    }

    // Lines Model //
    Graphics::Model Graphics::lines_model(const std::pair<glm::vec2, glm::vec2>& line_segment, const glm::vec3& clr_vtx)
    {
        // Use the start and end points of the line segment directly (no conversion to NDC)
        std::vector<glm::vec2> pos_vtx = {
            line_segment.first,   // Start point
            line_segment.second   // End point
        };

        // VBO and VAO setup
        Graphics::Model mdl;

        glCreateBuffers(1, &mdl.vbo_hdl);
        glNamedBufferStorage(mdl.vbo_hdl, sizeof(glm::vec2) * pos_vtx.size(), pos_vtx.data(), GL_DYNAMIC_STORAGE_BIT);


        glCreateVertexArrays(1, &mdl.vaoid);
        glEnableVertexArrayAttrib(mdl.vaoid, 0);
        glVertexArrayVertexBuffer(mdl.vaoid, 0, mdl.vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 0, 0);
        glBindVertexArray(0);

        mdl.primitive_type = GL_LINES;
        mdl.setup_shdrpgm(UE_vs, UE_fs);
        mdl.draw_cnt = static_cast<GLuint>(pos_vtx.size()); // number of vertices (2 for a line)
        mdl.primitive_cnt = 1; // only one line
        mdl.color = clr_vtx;

        // **Initialize the transformation matrices** (NEW)
        mdl.modelMatrix = glm::mat4(1.0f); // Identity matrix for the model
        mdl.viewMatrix = glm::mat4(1.0f);  // Identity matrix for the view (for 2D)
        mdl.projectionMatrix = glm::ortho(0.0f, 1600.0f, 900.0f, 0.0f); // 1600x900 viewport, non-NDC
        mdl.alpha = 1.0f;

        return mdl;
    }

    // Circle Model //
    Graphics::Model Graphics::trifans_model(float radius, float centerX, float centerY, const glm::vec3& clr_vtx)
    {
        int segments = 100; //Set value to make things smooth

        // Create a vector to hold the vertex positions
        std::vector<glm::vec2> pos_vtx;
        // Start with the center of the circle
        pos_vtx.push_back(glm::vec2(centerX, centerY));

        // Calculate the circle's vertices
        for (int i = 0; i <= segments; ++i)
        {
            float angle = 2.0f * glm::pi<float>() * i / segments; // Angle for each segment
            float x = centerX + radius * cos(angle); // X coordinate
            float y = centerY + radius * sin(angle); // Y coordinate
            pos_vtx.push_back(glm::vec2(x, y)); // Add vertex to vector
        }

        // VBO and VAO setup
        Graphics::Model mdl;

        glCreateBuffers(1, &mdl.vbo_hdl);
        glNamedBufferStorage(mdl.vbo_hdl, sizeof(glm::vec2) * pos_vtx.size(), pos_vtx.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &mdl.vaoid);
        glEnableVertexArrayAttrib(mdl.vaoid, 0);
        glVertexArrayVertexBuffer(mdl.vaoid, 0, mdl.vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 0, 0);
        glBindVertexArray(0);

        mdl.setup_shdrpgm(UE_vs, UE_fs);
        mdl.primitive_type = GL_TRIANGLE_FAN;
        mdl.color = clr_vtx; // Store the color in the model
        mdl.draw_cnt = static_cast<GLuint>(pos_vtx.size());
        mdl.primitive_cnt = mdl.draw_cnt - 2;
        mdl.alpha = 1.0f;

        // **Initialize the transformation matrices** (NEW)
        mdl.modelMatrix = glm::mat4(1.0f); // Identity matrix for the model
        mdl.viewMatrix = glm::mat4(1.0f);  // Identity matrix for the view (for 2D)
        // Set up the orthographic projection for your viewport
        mdl.projectionMatrix = glm::ortho(0.0f, projWidth, projHeight, 0.0f); //1600x900 viewport

        return mdl;
    }

    // Square Model //
    Graphics::Model Graphics::sqaure_model(glm::vec2 position, float width, float height, const glm::vec3& clr_vtx)
    {
        float half_width = width / 2.0f;
        float half_height = height / 2.0f;

        std::vector<glm::vec2> pos_vtx = {
            glm::vec2(position.x - half_width, position.y + half_height), // Top-left
            glm::vec2(position.x + half_width, position.y + half_height), // Top-right
            glm::vec2(position.x + half_width, position.y - half_height), // Bottom-right
            glm::vec2(position.x - half_width, position.y - half_height)  // Bottom-left
        };
        // Indices for drawing two triangles to form the square
        std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

        // Create VBO and EBO for the vertices and indices


        // Create a VAO
        Graphics::Model mdl;
        glCreateVertexArrays(1, &mdl.vaoid);

        glCreateBuffers(1, &mdl.ebo_hdl);
        glNamedBufferStorage(mdl.ebo_hdl, sizeof(GLuint) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);


        glCreateBuffers(1, &mdl.vbo_hdl);
        glNamedBufferStorage(mdl.vbo_hdl, sizeof(glm::vec2) * pos_vtx.size(), pos_vtx.data(), GL_DYNAMIC_STORAGE_BIT);


        // Bind the position buffer
        glEnableVertexArrayAttrib(mdl.vaoid, 0);
        glVertexArrayVertexBuffer(mdl.vaoid, 0, mdl.vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 0, 0);

        // Bind the element buffer for drawing
        glVertexArrayElementBuffer(mdl.vaoid, mdl.ebo_hdl);
        glBindVertexArray(0);

        // Set model properties
        mdl.primitive_type = GL_TRIANGLES;
        mdl.setup_shdrpgm(UE_vs, UE_fs);
        mdl.color = clr_vtx;  // Use this color in your fragment shader to apply it
        mdl.draw_cnt = static_cast<GLuint>(indices.size());
        mdl.primitive_cnt = mdl.draw_cnt / 3;
        mdl.alpha = 1.0f;

        // **Initialize the transformation matrices** (NEW)
        mdl.modelMatrix = glm::mat4(1.0f); // Identity matrix for the model
        mdl.viewMatrix = glm::mat4(1.0f);  // Identity matrix for the view (for 2D)
        mdl.projectionMatrix = glm::ortho(0.0f, projWidth, projHeight, 0.0f); // 1600x900 viewport, NDC

        return mdl;
    }

    // Create Mesh //
    Graphics::Model Graphics::createMesh(const std::vector<glm::vec2>& vtx_coord,
        const std::vector<glm::vec2>& txt_coords,
        const glm::vec3& clr_vtx,
        const std::string& meshName,
        const std::string& textureName) // Add texture path
    {
        GLuint textureID = GlobalAssetManager.UE_LoadTextureToOpenGL(textureName);
        
        std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

        // Create buffers and set up VAO (same as before)
        // Create model and VAO
        Graphics::Model mdl;

        glCreateBuffers(1, &mdl.vbo_hdl);
        glNamedBufferStorage(mdl.vbo_hdl, sizeof(glm::vec2) * vtx_coord.size(), vtx_coord.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &mdl.tex_vbo_hdl);
        glNamedBufferStorage(mdl.tex_vbo_hdl, sizeof(glm::vec2) * txt_coords.size(), txt_coords.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateBuffers(1, &mdl.ebo_hdl);
        glNamedBufferStorage(mdl.ebo_hdl, sizeof(GLuint) * indices.size(), indices.data(), GL_DYNAMIC_STORAGE_BIT);

        glCreateVertexArrays(1, &mdl.vaoid);

        // Vertex positions
        glEnableVertexArrayAttrib(mdl.vaoid, 0);
        glVertexArrayVertexBuffer(mdl.vaoid, 0, mdl.vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 0, 0);

        // Texture coordinates
        glEnableVertexArrayAttrib(mdl.vaoid, 1);
        glVertexArrayVertexBuffer(mdl.vaoid, 1, mdl.tex_vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 1, 1);

        // Index buffer
        glVertexArrayElementBuffer(mdl.vaoid, mdl.ebo_hdl);
        glBindVertexArray(0);

        // Setup the shader program
        mdl.setup_shdrpgm(UE_vs, UE_fs);
        mdl.color = clr_vtx;
        mdl.textureID = textureID; // Store the texture ID in the model
        mdl.primitive_type = GL_TRIANGLES;
        mdl.draw_cnt = static_cast<GLuint>(indices.size());
        mdl.primitive_cnt = mdl.draw_cnt / 3;

        // **Initialize the transformation matrices** (NEW)
        mdl.modelMatrix = glm::mat4(1.0f); // Identity matrix for the model
        mdl.viewMatrix = glm::mat4(1.0f);  // Identity matrix for the view (for 2D)
        mdl.projectionMatrix = glm::ortho(0.0f, projWidth, projHeight, 0.0f); // 1600x900 viewport, NDC

        // Store mesh in the container
        meshes[meshName] = mdl;

        return mdl;
    }

    // Getting Mesh //
    Graphics::Model& Graphics::getMesh(const std::string& name)
    {
        return meshes[name];  // Ensure that the mesh exists
    }

    //  Mesh //
    // Integrated texture coordinate update for animation
    void Graphics::drawMeshWithAnimation(Graphics::Model& mdl, int currFrame, int cols, int rows)
    {
        // Calculate frame-specific texture coordinates
        int col = currFrame % cols;
        int row = currFrame / cols;

        float frameWidth = 1.0f / cols;
        float frameHeight = 1.0f / rows;

        float u_min = col * frameWidth;
        float v_min = row * frameHeight;
        float u_max = u_min + frameWidth;
        float v_max = v_min + frameHeight;

        // Update the texture coordinates dynamically for the frame
        std::vector<glm::vec2> txt_coords = {
            { u_min, v_min },  // Bottom-left
            { u_max, v_min },  // Bottom-right
            { u_max, v_max },  // Top-right
            { u_min, v_max }   // Top-left
        };

        // Update the texture VBO with new texture coordinates
        GLuint tex_vbo_hdl;
        glCreateBuffers(1, &tex_vbo_hdl);
        glNamedBufferStorage(tex_vbo_hdl, sizeof(glm::vec2) * txt_coords.size(), txt_coords.data(), GL_DYNAMIC_STORAGE_BIT);

        glEnableVertexArrayAttrib(mdl.vaoid, 1);
        glVertexArrayVertexBuffer(mdl.vaoid, 1, tex_vbo_hdl, 0, sizeof(glm::vec2));
        glVertexArrayAttribFormat(mdl.vaoid, 1, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(mdl.vaoid, 1, 1);

        // Bind and draw the mesh as usual
        glBindVertexArray(mdl.vaoid);
        glBindVertexArray(0);

        // Cleanup the buffer
        glDeleteBuffers(1, &tex_vbo_hdl);
        mdl.primitive_type = GL_TRIANGLES;
    }

    void Graphics::DrawDebugBox(const glm::vec2& center, float width, float height)
    {
        // Calculate half dimensions
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        // Define the corners of the box
        glm::vec2 topLeft(center.x - halfWidth, center.y + halfHeight);
        glm::vec2 topRight(center.x + halfWidth, center.y + halfHeight);
        glm::vec2 bottomLeft(center.x - halfWidth, center.y - halfHeight);
        glm::vec2 bottomRight(center.x + halfWidth, center.y - halfHeight);

        Graphics::Model& model = getMesh("sprite"); // Use for mesh

        // Loads the texture if it�s not loaded into map
        if (textures.find("Hitbox") == textures.end())
        {
            textures["Hitbox"] = GlobalAssetManager.UE_LoadTextureToOpenGL("Hitbox");
        }

        model.textureID = textures["Hitbox"]; // Assign loaded texture ID to model

        // TRANSLATE, ROTATE, SCALE
        glm::vec2 translation(center.x, center.y);
        float rotation = 0.f;
        glm::vec2 scale(width, height);

        // SET MATRIX
        model.modelMatrix = Graphics::calculate2DTransform(translation, rotation, scale);

        // Set color and alpha 
        model.color = { 1.0f ,1.0f,1.0f };
        model.alpha = 1.0f;

        // Draw the model
        model.draw();
    }

    // Setting Up Shader Programs //
    void Graphics::Model::setup_shdrpgm(std::string const& vtx_shdr,
        std::string const& frag_shdr)
    {
        //compile shaders
        if (!shdr_pgm.CompileShaderFromString(GL_VERTEX_SHADER, vtx_shdr))
        {
            std::cout << "Vertex shader failed to compile: ";
            std::cout << shdr_pgm.GetLog() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (!shdr_pgm.CompileShaderFromString(GL_FRAGMENT_SHADER, frag_shdr))
        {
            std::cout << "Fragment shader failed to compile: ";
            std::cout << shdr_pgm.GetLog() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (!shdr_pgm.Link()) //Linking shader 
        {
            std::cout << "Shader program failed to link!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        if (!shdr_pgm.Validate()) //ensuring that it validates
        {
            std::cout << "Shader program failed to validate!" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    //For ImGUI 
    GLuint Graphics::CreateFramebuffer(int width, int height, GLuint& texture, GLuint& rbo_fb)
    {
        GLuint framebuffer;

        // Generate and bind framebuffer
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Create a color texture to render to
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        // Create a renderbuffer for depth and stencil
        glGenRenderbuffers(1, &rbo_fb);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_fb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_fb);

        // Check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return 0; // Indicate failure
        }

        // Unbind framebuffer to revert to default
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return framebuffer;
    }

    //FILE OPENING / SAVING
    std::string Graphics::OpenFileDialog()
    {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Scene Files\0*.scene;*.json;*.xml\0All Files\0*.*\0\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            std::string fullPath = std::string(ofn.lpstrFile);

            // Check if the path contains "src/"
            size_t pos = fullPath.find("src/");
            if (pos != std::string::npos) {
                // If found, remove the "src/" part of the path
                std::string relativePath = fullPath.substr(pos + 4);  // Skip "src/" (4 characters)

                // Replace backslashes with forward slashes
                std::replace(relativePath.begin(), relativePath.end(), '\\', '/');

                // Return the relative path (directly inside the "src" directory)
                return relativePath;
            }
            else {
                // If "src/" is not found, return the full path with backslashes converted
                std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                return fullPath;
            }
        }
        return "";
    }

    std::string Graphics::SaveFileDialog()
    {
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Scene Files\0*.json;*.scene;*.xml\0All Files\0*.*\0\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;

        // Change flag to allow new file creation and prompt for overwrite
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

        if (GetSaveFileNameA(&ofn) == TRUE)
        {
            std::string fullPath(ofn.lpstrFile);
            std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

            // Ensure .json extension if not already provided
            if (fullPath.substr(fullPath.find_last_of(".") + 1) != "json")
            {
                fullPath += ".json"; // Add .json extension
            }
            return fullPath;
        }
        return "";
    }

    // Error Detection for Audio
    void Graphics::RenderErrorPopup()
    {
        if (showErrorPopup)
        {
            ImGui::OpenPopup("Error");  // Open popup when error flag is set
        }
        if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", errorMessage.c_str());  // Display error message
            ImGui::Separator();
            if (ImGui::Button("Close"))
            {
                ImGui::CloseCurrentPopup();
                showErrorPopup = false;  // Reset the error flag
            }
            ImGui::EndPopup();
        }
    }

    // Edwin , drag and drop Texture/Audio
    void Graphics::DropCallback(GLFWwindow* window, int count, const char** paths)
    {
        (void)window;
        for (int i = 0; i < count; i++) 
        {
            std::string callbackFilePath = paths[i];
            std::string extension = callbackFilePath.substr(callbackFilePath.find_last_of(".") + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);  // Convert to lowercase for case-insensitive comparison

            // Check if the file is a texture file (e.g., .png, .jpg, .bmp, etc.)
            if (extension == "png" || extension == "jpg" || extension == "jpeg" || extension == "bmp" || extension == "tga" || extension == "dds") 
            {
                std::string textureName = callbackFilePath.substr(callbackFilePath.find_last_of("/\\") + 1);  // Get filename
                std::string textureNameWithoutExt = textureName.substr(0, textureName.find_last_of("."));

                auto it = textureAssets.find(textureNameWithoutExt);
                if (it == textureAssets.end()) 
                {
                    GlobalAssetManager.UE_AddTexture(textureNameWithoutExt, callbackFilePath);  // Add to texture asset manager
                    std::cout << "Texture added: " << textureNameWithoutExt << " from path: " << callbackFilePath << std::endl;
                }
                else 
                {
                    std::cout << "Texture already exists: " << textureNameWithoutExt << std::endl;
                }
            }
            // Check if the file is an audio file (e.g., .mp3, .wav, .ogg, etc.)
            else if (extension == "mp3" || extension == "wav" || extension == "flac") 
            {
                std::string audioName = callbackFilePath.substr(callbackFilePath.find_last_of("/\\") + 1);  // Get filename
                auto it = audioAssets.find(audioName);
                if (it == audioAssets.end()) 
                {
                    GlobalAssetManager.UE_AddAudio(callbackFilePath);  // Add to audio asset manager
                    std::cout << "Audio file added: " << audioName << " from path: " << callbackFilePath << std::endl;
                }
                else 
                {
                    std::cout << "Audio file already exists: " << audioName << std::endl;
                }
            }
            else if (extension == "ogg")
            {
                errorMessage = "Unsupported file type: ." + extension + "\nPlease use supported formats (.mp3, .wav, .flac).";
                showErrorPopup = true;      // Unsupported .ogg file type - set error message and trigger popup
            }
            else 
            {
                std::cout << "Unsupported file type: " << extension << " for file: " << callbackFilePath << std::endl;      // Handle other file types (non-texture and non-audio files)
            }
        }
    }

    // Get the current working directory
    std::wstring Graphics::GetCurrentWorkingDirectory()
    {
        wchar_t buffer[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, buffer);
        return std::wstring(buffer);
    }

    // Change the current working directory
    void Graphics::ChangeWorkingDirectory(const std::wstring& newDirectory)
    {
        if (SetCurrentDirectory(newDirectory.c_str()) == 0)
        {
            std::wcerr << L"Failed to change directory to: " << newDirectory << std::endl;
        }
        else
        {
            std::wcout << L"Changed Working Directory: " << newDirectory << std::endl;
        }
    }

    // Extract the directory from a file path
    std::string Graphics::ExtractBasePath(const std::string& baseFilePath)
    {
        size_t lastSlash = baseFilePath.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            // Move back to one level above the 'SceneFiles' directory
            return baseFilePath.substr(0, lastSlash);  // Extract parent directory
        }
        return ".";  // Return current directory if no slashes are found
    }

    std::wstring Graphics::ExtractParentDirectory(const std::wstring& directory, int levels)
    {
        std::wstring result = directory;
        for (int i = 0; i < levels; ++i)
        {
            size_t lastSlash = result.find_last_of(L"/\\");
            if (lastSlash != std::wstring::npos)
            {
                result = result.substr(0, lastSlash);
            }
            else
            {
                // If we can't go up anymore, return the current result
                break;
            }
        }
        return result;
    }
    
    bool Graphics::IsMouseOutsideViewport(ImVec2 viewportMin, ImVec2 viewportMax)
    {
        ImVec2 mousePos = ImGui::GetMousePos();
        return mousePos.x < viewportMin.x || mousePos.x > viewportMax.x ||
            mousePos.y < viewportMin.y || mousePos.y > viewportMax.y;
    }

    /* ALL USAGE FUNCTIONS BELOW */
    // ---------------------------- //

    void Graphics::DrawCircle(float radius, float centerX, float centerY, const glm::vec3& clr_vtx)
    {
        Graphics::models.emplace_back(Graphics::trifans_model(radius, centerX, centerY, clr_vtx));
    }

    void Graphics::DrawLine(const std::pair<glm::vec2, glm::vec2>& line_segment, const glm::vec3& clr_vtx)
    {
        Graphics::models.emplace_back(Graphics::lines_model(line_segment, clr_vtx));
    }

    void Graphics::DrawPoint(const glm::vec2& coordinates, const glm::vec3& clr_vtx)
    {
        Graphics::models.emplace_back(Graphics::points_model(coordinates, clr_vtx));
    }

    void Graphics::DrawSquare(glm::vec2 position, float width, float height, const glm::vec3& clr_vtx)
    {
        Graphics::models.emplace_back(Graphics::sqaure_model(position, width, height, clr_vtx));
    }

    glm::mat4 Graphics::calculate2DTransform(const glm::vec2& translation, float rotation, const glm::vec2& scale)
    {
        // Start with an identity matrix
        glm::mat4 model = glm::mat4(1.0f);

        // Apply translation in X and Y
        model = glm::translate(model, glm::vec3(translation, 0.0f));

        // Apply rotation (around the Z-axis)
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));

        // Apply scaling in X and Y
        model = glm::scale(model, glm::vec3(scale, 1.0f));

        return model;
    }

    //CAMERA FUNCTIONS
    glm::mat4 Graphics::Camera::getViewMatrix() const
    {
        // Identity matrix
        glm::mat4 view = glm::mat4(1.0f);

        // Step 1: Translate to zoom point (the position around which you want to zoom)
        view = glm::translate(view, glm::vec3(-position, 0.0f));

        // Step 2: Apply zoom (scale)
        view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));

        // Step 3: Translate back by the camera position
        view = glm::translate(view, glm::vec3(position, 0.0f));

        return view;
    }

    void Graphics::Camera::move(const glm::vec2& delta)
    {
        position += delta; // Update camera position
    }

    void Graphics::Camera::setZoom(float newZoom)
    {
        zoom = newZoom;
    }

    void Graphics::Camera::setPosition(const glm::vec2& newPosition)
    {
        position = newPosition;
    }

    // CLEANING AND FREE-ING OF MEMORY //
    void Graphics::Model::cleanup()
    {
        // Delete the VAO
        if (vaoid != 0)
        {
            glDeleteVertexArrays(1, &vaoid);
            vaoid = 0;
        }

        // Delete the VBO for vertex positions
        if (vbo_hdl != 0)
        {
            glDeleteBuffers(1, &vbo_hdl);
            vbo_hdl = 0;
        }

        // Delete the EBO
        if (ebo_hdl != 0)
        {
            glDeleteBuffers(1, &ebo_hdl);
            ebo_hdl = 0;
        }

        // Delete the VBO for texture coordinates
        if (tex_vbo_hdl != 0)
        {
            glDeleteBuffers(1, &tex_vbo_hdl);
            tex_vbo_hdl = 0;
        }

        // Delete the texture (if applicable)
        if (textureID != 0)
        {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }

        // Delete the shader program
        shdr_pgm.DeleteShaderProgram();
    }

    // Function to start the shake
    void Graphics::startShake(float duration, float magnitude) 
    {
        shakeDuration = duration;
        shakeMagnitude = magnitude;
    }

    void AssignTag(Framework::Entity entityID, std::string Tag) {
        //brute force tag for animation once per entity
        if (!ecsInterface.HasTag(entityID, Tag)) {
            ecsInterface.AddTag(entityID, Tag);
            std::cout << "Added tag" << Tag;
        }
    }

    // Function to update shake effect
    void Graphics::updateShake(float deltaTime) 
    {
        if (shakeDuration > 0.0f) {
            // Random offsets between -shakeMagnitude and +shakeMagnitude
            shakeOffsetX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * shakeMagnitude;
            shakeOffsetY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * shakeMagnitude;

            shakeDuration -= deltaTime; // Reduce shake duration
            if (shakeDuration <= 0.0f) {
                // Reset offsets when the shake ends
                shakeOffsetX = 0.0f;
                shakeOffsetY = 0.0f;
            }
        }
    }

    void Graphics::RenderFPS(float projWidth, float projHeight) {
        // Set active font (assuming you have a default font)
        fontSystem.SetActiveFont("Salmon"); // Change to your actual font

        // Position for displaying FPS (e.g., top-left corner)
        glm::vec2 textPosition = glm::vec2(100.f, 100.f); // Adjust as needed

        // Set up an orthographic projection for 2D text rendering
        glm::mat4 projection = glm::ortho(0.0f, projWidth, projHeight, 0.0f);

        // Convert FPS to string
        std::string fpsText = "FPS: " + std::to_string(engineState.GlobalFPS);

        // Render the text
        fontSystem.RenderText(fpsText, textPosition.x, textPosition.y, 1, glm::vec3(1.0f, 0.0f, 0.0f), projection);
    }

    //// IMGUI BELOW
    void Graphics::showImGUI()
    {
        // Start a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a full-screen dockspace
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // Add flags to prevent moving and resizing
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        // Begin the main dockspace window
        ImGui::Begin("DockSpace Demo", nullptr, windowFlags);
        ImGui::PopStyleVar(2);

        // Bind the default framebuffer again
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Create a dockspace ID
        ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");

        // Create the dockspace
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        // Optionally add a menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Unnamed Studio Game Engine"))
            {
                if (ImGui::MenuItem("Open"))
                {
                    filePath = Graphics::OpenFileDialog();      // Get the file path from the OpenFileDialog   
                    if (!filePath.empty())                      // Ensure the file path is not empty before proceeding
                    {
                        // Store the current directory before changing
                        std::wstring originalDir = Graphics::GetCurrentWorkingDirectory();
                        std::wcout << L"Original Directory: " << originalDir << std::endl;

                        // Extract the directory of the scene file
                        std::string newDirectory = Graphics::ExtractBasePath(filePath);
                        std::cout << "New Directory (Extracted): " << newDirectory << std::endl;

                        // Change to the directory of the file
                        Graphics::ChangeWorkingDirectory(std::wstring(newDirectory.begin(), newDirectory.end()));
                        std::wcout << L"Changed Working Directory: " << Graphics::GetCurrentWorkingDirectory() << std::endl;

                        // Load the entities
                        ecsInterface.ClearEntities();
                        entityAssets.clear();
                        GlobalAssetManager.UE_LoadEntities(filePath);

                        // Revert to the directory two levels above the original directory
                        std::wstring parentDir = Graphics::ExtractParentDirectory(originalDir, 2);
                        Graphics::ChangeWorkingDirectory(parentDir);
                        std::wcout << L"Reverted to Parent's Parent Directory: " << Graphics::GetCurrentWorkingDirectory() << std::endl;
                    }
                }

                if (ImGui::MenuItem("Save"))
                {
                    std::string savePath = SaveFileDialog();
                    if (!savePath.empty())
                    {
                        GlobalEntityAsset.SerializeEntities(savePath);
                    }
                    else
                    {
                        std::cout << "Save path selected: " << savePath << std::endl;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        ImGui::End();
        RenderErrorPopup();
    }
}