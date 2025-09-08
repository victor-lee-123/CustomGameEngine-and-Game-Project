///////////////////////////////////////////////////////////////////////////////
///
///	@file GraphicsWindows.h
/// 
/// @brief Header files for GraphicsWindows.cpp
///	
///	@Authors: Victor Lee / Joshua Sim
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////


#pragma once
#include <glew.h>
#include "System.h"
#include <GLFW/glfw3.h>
#include <string>
#include "Core.h"
#include <InputHandler.h>

#include "AssetManager.h"

namespace Framework {

    class GraphicsWindows : public ISystem {
    public:
        // Constructor to initialize window properties
        //GraphicsWindows(int width, int height, const std::string& title, CoreEngine* CorePointer);
        GraphicsWindows(const Window::WindowConfig& config, CoreEngine* EnginePointer);
        
        // Destructor to clean up resources
        ~GraphicsWindows();

        // ISystem interface implementations
        void Initialize() override;
        void Update(float deltaTime) override;
        std::string GetName() override { return "GraphicsWindows"; }

        static InputHandler* InputHandler; //Instance

        // Check if the window is still open
        bool IsWindowOpen() const;

        //Getter
        GLFWwindow* GetWindow();

        //FPS Counter
        static float FPS;

        //Getting viewport resizing callback
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

        int getWidth() const { return screenWidth;}
        int getHeight() const { return screenHeight;}

        static bool fullscreen;
        static bool isQuit;
        static int result2;
        static bool result_menu;
        static bool isMainMenu;
        static bool wasFullscreen;

    private:
        GLFWwindow* window;  // GLFW window pointer
        int screenWidth, screenHeight;  // Window size
        std::string windowTitle;  // Window title
        CoreEngine* CorePointer;

        bool isInitialized;  // Tracks whether the system is initialized
    };
}
