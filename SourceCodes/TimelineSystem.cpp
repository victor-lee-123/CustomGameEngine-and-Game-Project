///////////////////////////////////////////////////////////////////////////////
///
/// @file TimelineSystem.cpp
/// 
/// @brief Implementation of the TimelineSystem class, which manages timeline 
///        animations and transitions for entities in the game engine. This class 
///        provides functionality to smoothly handle transitions like fades, slides, etc.
///  
/// @authors 
/// Joshua Sim Yue Chen
/// 
/// @copyright 
/// 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <iostream>
#include "TimelineSystem.h"
#include "Coordinator.h"
#include "ComponentList.h"
#include "Graphics.h"
#include "SceneManager.h"
#include "EngineState.h"

extern Framework::Coordinator ecsInterface;

namespace Framework {
    TimelineSystem GlobalTimelineSystem;

    void TimelineSystem::Initialize() {

        // Register TimelineComponent with the ECS
        ecsInterface.RegisterComponent<TimelineComponent>();

        // Define the system's signature (entities must have TimelineComponent)
        Signature signature;
        signature.set(ecsInterface.GetComponentType<TimelineComponent>());
        ecsInterface.SetSystemSignature<TimelineSystem>(signature);
        std::cout << "TimelineSystem initialized." << std::endl;
    }
    void TimelineSystem::Update(float deltaTime) {
        // Ensure the system only runs in Play mode
        if (!engineState.IsPlay()) {
            return; // Do not run the timeline system
        }

        // Iterate over all entities with TimelineComponent
        for (auto const& entity : mEntities) {
            auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
            auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
            LayerComponent& layerComponent = ecsInterface.GetComponent<LayerComponent>(entity);
            (void)transform; // Avoids unused variable warning

            // Skip rendering based on layer visibility
            if (!engineState.layerVisibility[layerComponent.layerID]) {
                continue;  // Skip all entities in this layer
            }

            // Ensure the tag is applied only once
            if (!ecsInterface.HasTag(entity, timeline.TimelineTag)) {
                ecsInterface.AddTag(entity, timeline.TimelineTag);
            }

            // Skip inactive timelines
            if (!timeline.Active) {
                continue;
            }

            // **Handle Transition In**
            if (timeline.IsTransitioningIn) {
                timeline.DelayAccumulated += deltaTime; // Accumulate delay

                if (timeline.DelayAccumulated < timeline.TransitionInDelay) {
                    continue; // Wait until transition in delay is over
                }

                // Start the transition in
                timeline.InternalTimer += deltaTime;
                if (timeline.TransitionIn) {
                    timeline.TransitionIn(entity, timeline.InternalTimer);
                }

                // Check if transition in is complete
                if (timeline.InternalTimer >= timeline.TransitionDuration) {
                    timeline.IsTransitioningIn = false; // Move to Transition Out
                    timeline.InternalTimer = 0.0f;      // Reset timer
                    timeline.DelayAccumulated = 0.0f;   // Reset in delay accumulation
                }
            }

            // **Handle Transition Out**
            else {
                timeline.DelayOutAccumulated += deltaTime; // Accumulate out delay

                if (timeline.DelayOutAccumulated < timeline.TransitionOutDelay) {
                    continue; // Wait until transition out delay is over
                }

                // Start the transition out
                timeline.InternalTimer += deltaTime;
                if (timeline.TransitionOut) {
                    timeline.TransitionOut(entity, timeline.InternalTimer);
                }

                // Check if transition out is complete
                if (timeline.InternalTimer >= timeline.TransitionDuration) {
                    timeline.Active = false;  // Deactivate timeline after transition out
                }
            }
        }
    }


    void TimelineSystem::ToggleActive(std::string TimelineTag) {
        bool activated = false;

        for (auto const& entity : mEntities) {
            auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
            if (timeline.TimelineTag == TimelineTag) {
                timeline.Active = true;
                activated = true;
                std::cout << "Timeline with tag '" << TimelineTag << "' activated for entity " << entity << ".\n";
            }
        }
        if (!activated) {
            std::cerr << "Warning: No timeline with tag '" << TimelineTag << "' found to activate.\n";
        }
    }

}

