///////////////////////////////////////////////////////////////////////////////
///
///	@file GraphicsWindows.cpp
/// 
/// @brief Creates graphics windows for usage in viewport settings
///	
///	@Authors: Victor Lee / Joshua Sim 
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"

#include <Windows.h>
#include "GraphicsWindows.h"
#include "Core.h"
#include "EngineState.h"

namespace Framework {


    float GraphicsWindows::FPS{};
    InputHandler* GraphicsWindows::InputHandler{};
    bool isQuit = false;
    bool isMainMenu = false;
    bool result_menu = false;
    bool GraphicsWindows::fullscreen = true;
    // Add this variable to track the previous state
    int result{}, result2{};
    bool GraphicsWindows::wasFullscreen = false;
    /**
     * @brief Constructor for GraphicsWindows.
     *
     * Initializes a window using GLFW with the given configuration settings and links it to the core engine.
     *
     * @param config The configuration settings for the window (width, height, and title).
     * @param CorePointer A pointer to  the core engine, used to signal when the window should close.
     */
    GraphicsWindows::GraphicsWindows(const Window::WindowConfig& config, CoreEngine* CorePointer)
        : screenWidth(config.x), screenHeight(config.y), windowTitle(config.programName),
        window(nullptr), isInitialized(false), CorePointer(CorePointer) {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return;
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(screenWidth, screenHeight, windowTitle.c_str(), nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }

        glfwMakeContextCurrent(window);  // Make the OpenGL context current
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
        glViewport(0, 0, screenWidth, screenHeight);

        isInitialized = true;
    }

    /**
    * @brief Destructor for GraphicsWindows.
    *
    * Cleans up resources by destroying the GLFW window and terminating GLFW.
    */
    GraphicsWindows::~GraphicsWindows() {
        if (window) {
            glfwDestroyWindow(window);  // Clean up the GLFW window
            std::cout << "Window destroyed" << std::endl;
        }

        glfwTerminate();  // Terminate GLFW
    }

    /**
   * @brief Initializes the GraphicsWindows system.
   *
   * Currently, this function does not perform additional setup since initialization happens in the constructor.
   */
    void GraphicsWindows::Initialize() {

        InputHandler = InputHandler::GetInstance();
    }

    /**
      * @brief Updates the GraphicsWindows system.
      *
      * Renders content to the window, calculates FPS, and checks for window close events.
      *
      * @param deltaTime The time difference (in seconds) since the last frame update.
      */
    void GraphicsWindows::Update(float deltaTime) {

        (void)deltaTime;

        //testing if windows are being seen or not
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        {
            //GlobalAudio.UE_PauseAllAudio();
        }
        else
        {
            //GlobalAudio.UE_ResumeAllAudio();
        }

        if (InputHandler->IsKeyPressed(GLFW_KEY_MINUS))
        {  // Check if 'N' is pressed to toggle fullscreen
            fullscreen = !fullscreen;  // Toggle the fullscreen flag
        }

        // Get the current monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

        if (fullscreen && !wasFullscreen)
        {  // If fullscreen is enabled
            if (videoMode)
            {
                // Set the window to fullscreen
                glfwSetWindowMonitor(window, monitor, 0, 0, videoMode->width, videoMode->height, GLFW_DONT_CARE);
                //glViewport(0, 0, videoMode->width, videoMode->height);
            }
        }
        else if (!fullscreen && wasFullscreen)
        {  // If windowed mode
            // Set the window to windowed mode (use your original window size here)
            glfwSetWindowMonitor(window, nullptr, 100, 100, screenWidth/1.2, screenHeight/1.2, 0);  // Example position and size
            glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
            //glViewport(0, 0, screenWidth, screenHeight);
        }

        // Update the previous state
        wasFullscreen = fullscreen;

        // Update the viewport for the new window size (fullscreen or windowed)
        glfwMakeContextCurrent(window);  // Make the OpenGL context current
        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

        // Input Example
        if (InputHandler->IsKeyPressed(GLFW_MOUSE_BUTTON_LEFT))
            //std::cout << "CLICK";

            if (!isInitialized)
            {
                return;
            }
        // Check if the window should close
        if (glfwWindowShouldClose(window)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);  // Flag to close the window
            CorePointer->EndGameLoop();
        }

        // FPS update using glfwGetTime
        static double lastTime = glfwGetTime();  // Record the last time
        static double accumulatedTime = 0.0;
        static int frameCount = 0;

        double currentTime = glfwGetTime();  // Get the current time in seconds
        double elapsedTime = currentTime - lastTime;  // Calculate elapsed time
        lastTime = currentTime;

        // Increment the frame count and accumulate elapsed time
        frameCount++;
        accumulatedTime += elapsedTime;

        // Update FPS every second (when accumulated time reaches 1 second)
        if (accumulatedTime >= 1.0)
        {
            // Calculate FPS as frames per second
            GraphicsWindows::FPS = (float)(frameCount / accumulatedTime);

            engineState.GlobalFPS = GraphicsWindows::FPS;

            // Update the window title with FPS and boolean flags as true/false
            std::string title = "Corvaders";
            glfwSetWindowTitle(window, title.c_str());

            // Reset the accumulated time and frame count
            accumulatedTime = 0.0;
            frameCount = 0;
        }

        glfwSwapBuffers(window);

        if (isQuit == true)
        {
            // Use MB_TOPMOST to ensure the message box appears in front of the fullscreen window
            result = MessageBoxA(
                NULL,                          // Parent window handle (NULL for the desktop)
                "Are you sure you want to quit?", // Message text
                "Quit",                        // Title of the message box
                MB_YESNO | MB_ICONQUESTION | MB_TOPMOST // Add the MB_TOPMOST flag
            );
            isQuit = false;
        }

        if (result == IDYES)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (isMainMenu == true)
        {
            // Use MB_TOPMOST to ensure the message box appears in front of the fullscreen window
            result2 = MessageBoxA(
                NULL,                          // Parent window handle (NULL for the desktop)
                "Are you sure?", // Message text
                "Main Menu",                        // Title of the message box
                MB_YESNO | MB_ICONQUESTION | MB_TOPMOST // Add the MB_TOPMOST flag
            );
            isMainMenu = false;
        }

        if (result2 == IDYES)
        {
            Framework::GlobalSceneManager.TransitionToScene("Assets/Scene/MenuScene.json");
            result2 = IDNO;
        }
    }


    /**
     * @brief Checks if the window is still open.
     *
     * @return True if the window is open, false otherwise.
     */
    bool GraphicsWindows::IsWindowOpen() const {
        return !glfwWindowShouldClose(window);
    }

    /**
    * @brief Returns the GLFW window pointer.
    *
    * @return Pointer to the GLFW window.
    */
    GLFWwindow* GraphicsWindows::GetWindow() {
        return window;
    }

    void GraphicsWindows::framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        (void)window;
        // Update the viewport with the new window size
        glViewport(0, 0, width, height);
    }

}
