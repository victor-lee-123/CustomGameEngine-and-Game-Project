///////////////////////////////////////////////////////////////////////////////
///
/// @file TimelineSystem.h
/// 
/// @brief Implementation of the TimelineSystem class, which manages timeline-based 
///        transitions and animations for entities within the game engine.
///        This class provides functionality for handling transitions like fades, slides, etc.
///  
/// @authors 
/// Joshua Sim Yue Chen
/// 
/// @copyright 
/// 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once

#include "pch.h"
#include <string>
#include <functional>
#include "System.h"
#include "ComponentList.h"

namespace Framework {

    class TimelineSystem : public ISystem {
    public:
        // Initialize the system and register necessary components
        void Initialize() override;

        // Update all active timelines
        void Update(float deltaTime) override;

        // Get the name of the system
        std::string GetName() override { return "TimelineSystem"; }

         void ToggleActive(std::string TimelineTag);


    };

    // Declare a global instance of the TimelineSystem
    extern TimelineSystem GlobalTimelineSystem;

} // namespace Framework
