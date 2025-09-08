///////////////////////////////////////////////////////////////////////////////
///
/// @file TimelineBehavior.h
/// 
/// @brief This file contains the implementation of various behavior functions
///        for entities in the timeline system, including animations like 
///        sliding, fading, and scaling. These functions use the ECS Coordinator
///        to manage component access and entity interactions.
///  
/// @authors 
/// Joshua Sim Yue Chen
/// 
/// @copyright 
/// 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include <cstddef> // For std::byte
#include "pch.h"
#include <iostream>
#include "Coordinator.h"
#include "EngineState.h"
#include "LogicManager.h"
#include "ComponentList.h"
#include <vector>
#include "SceneManager.h"
#include "GraphicsWindows.h"
#include "cmath"


extern Framework::Coordinator ecsInterface;

constexpr double M_PI = 3.14159265358979323846;

// Example Timeline Events

float Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

void SlideInTransition(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Calculate progress as a fraction of the transition duration
    float progress = timer / timeline.TransitionDuration;

    // Ensure progress is clamped between 0.0 and 1.0
    progress = std::min(progress, 1.0f);

    // Lerp based on progress from starting position to the target position
    float startPosition = timeline.startPosition; // Example: off-screen starting position // 2400.0f
    float targetPosition = timeline.endPosition; // Example: target position in the center // 800
    transform.position.x = Lerp(startPosition, targetPosition, progress);

    if (progress >= 1.0f) {
        // Transition complete, switch to TransitionOut
        timeline.IsTransitioningIn = false;
        timeline.InternalTimer = 0.0f; // Reset the timer for TransitionOut

        //Time enemy to spawn after transition.
       Framework::engineState.SetPaused(false);
        // Logic to start the game
       Framework::GlobalSceneManager.TransitionToScene(Framework::GlobalSceneManager.Variable_Scene);
   
    }
}

void SlideOut(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Calculate progress as a fraction of the transition duration
    float progress = timer / timeline.TransitionDuration;

    // Ensure progress is clamped between 0.0 and 1.0
    progress = std::min(progress, 1.0f);

    // Lerp based on progress from starting position to the target position
    float startPosition = timeline.startPosition; // Example: off-screen starting position // 2400.0f
    float targetPosition = timeline.endPosition; // Example: target position in the center // 800
    transform.position.x = Lerp(startPosition, targetPosition, progress);

    if (progress >= 1.0f) {
        // Transition complete, switch to TransitionOut
        timeline.IsTransitioningIn = false;
        timeline.InternalTimer = 0.0f; // Reset the timer for TransitionOut

        // Framework::engineState.SetPaused(true);
    }

}

void SlideOutWarning(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Calculate progress as a fraction of the transition duration
    float progress = timer / timeline.TransitionDuration;

    // Ensure progress is clamped between 0.0 and 1.0
    progress = std::min(progress, 1.0f);

    // Lerp based on progress from starting position to the target position
    float startPosition = 960.0f; // Example: off-screen starting position // 2400.0f
    float targetPosition = -2000.0f; // Example: target position in the center // 800
    transform.position.x = Lerp(startPosition, targetPosition, progress);

    if (progress >= 1.0f) {
        // Transition complete, switch to TransitionOut
        GlobalAssetManager.UE_LoadPrefab("BossBar.json");
        timeline.IsTransitioningIn = false;
        progress = 0.0f;
        // Framework::engineState.SetPaused(true);
    }
}

void SlideInElastic(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Calculate progress as a fraction of the transition duration
    float progress = timer / timeline.TransitionDuration;

    // Ensure progress is clamped between 0.0 and 1.0
    progress = std::min(progress, 1.0f);

    // Lerp from startPosition to targetPosition
    float startPosition = timeline.startPosition; // Example: off-screen starting position
    float targetPosition = timeline.endPosition;  // Example: target position in the center

    // Introduce a bounce using a sine wave dampened over time
    float overshoot = 1.1f;  // Adjust this to control how much it overshoots
    float damping = 3.0f;    // Controls how quickly the bounce settles
    float bounceFactor = static_cast<float>(std::exp(-damping * progress) * std::sin(overshoot * M_PI * progress));

    // Apply bounce effect to the position
    transform.position.x = Lerp(startPosition, targetPosition, progress) + (bounceFactor * 20.0f);

    if (progress >= 1.0f) {
        // Transition complete, switch to TransitionOut

        timeline.IsTransitioningIn = false;
        timeline.InternalTimer = 0.0f; // Reset the timer for TransitionOut
    }
}

void SlideUp(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float progress = timer / timeline.TransitionDuration;
    progress = std::min(progress, 1.0f);

    float startPositionY = timeline.startPosition; // Example: Y position below screen (e.g., 1200)
    float targetPositionY = timeline.endPosition; // Example: final Y position (e.g., 600)

    transform.position.y = Lerp(startPositionY, targetPositionY, progress);

    if (progress >= 1.0f) 
    {
        timeline.IsTransitioningIn = false;
        Framework::engineState.SetPaused(false);
    }
}

void Credits(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float progress = timer / timeline.TransitionDuration;
    progress = std::min(progress, 1.0f);

    float startPositionY = timeline.startPosition; // Example: Y position below screen (e.g., 1200)
    float targetPositionY = timeline.endPosition; // Example: final Y position (e.g., 600)

    transform.position.y = Lerp(startPositionY, targetPositionY, progress);

    if (progress >= 1.0f)
    {
        // Loop: reset timer and start over
        timeline.InternalTimer = 0.0f;
        transform.position.y = timeline.startPosition;
        timeline.IsTransitioningIn = true;
    }
}


void SlideDiag(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float progress = timer / timeline.TransitionDuration;
    progress = std::min(progress, 1.0f);

    float startPositionX = timeline.startPosition; // Example: Bottom left X (e.g., -200)
    float targetPositionX = timeline.endPosition; // Example: final X position (e.g., 400)

    transform.position.x = Lerp(startPositionX, targetPositionX, progress);
    transform.position.y = Lerp(startPositionX / 1.77f, targetPositionX / 1.77f, progress);

    if (progress >= 1.0f)
    {
        timeline.IsTransitioningIn = false;
        Framework::engineState.SetPaused(false);
    }
}


void SlideInBounce(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Calculate progress as a fraction of the transition duration
    float progress = timer / timeline.TransitionDuration;

    // Ensure progress is clamped between 0.0 and 1.0
    progress = std::min(progress, 1.0f);

    // Lerp from startPosition to targetPosition (smooth slide-in motion)
    float startPosition = timeline.startPosition;
    float targetPosition = timeline.endPosition;
    transform.position.x = Lerp(startPosition, targetPosition, progress);

    // Apply bounce effect **only when progress is complete**
    if (progress >= 1.0f)
    {
        float bounceTime = timeline.InternalTimer; // Time after reaching the target
        float bounceFactor = static_cast<float>(std::exp(-4.0f * bounceTime) * std::sin(8.0f * M_PI * bounceTime));

        transform.position.x = targetPosition + (bounceFactor * 15.0f); // Bounce effect
        timeline.InternalTimer += 0.016f; // Assuming this function runs every frame (~60FPS)

        if (bounceTime > 1.0f)  // Stop bounce after 1 second
        {
            timeline.IsTransitioningIn = false;
            timeline.InternalTimer = 0.0f; // Reset timer for next transition
            Framework::engineState.SetPaused(false);
        }
    }
}


void SlideInWobbly(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float progress = timer / timeline.TransitionDuration;
    progress = std::min(progress, 1.0f);

    float startPosition = timeline.startPosition;
    float targetPosition = timeline.endPosition;

    // Wobble effect
    float wobbleFactor = std::sin(progress * 10.0f) * (1.0f - progress) * 20.0f;

    transform.position.x = Lerp(startPosition, targetPosition, progress) + wobbleFactor;

    if (progress >= 1.0f)
    {
        timeline.IsTransitioningIn = false;
        Framework::engineState.SetPaused(false);
    }
}

void SlideInCircular(Framework::Entity entity, float timer)
{
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float progress = timer / timeline.TransitionDuration;
    progress = std::min(progress, 1.0f);

    float centerX = 800.0f; // Midpoint of the screen
    float radius = 500.0f;  // Curve radius
    float angle = static_cast<float>(progress * (M_PI / 2)); // Moves in an arc

    transform.position.x = centerX + radius * std::cos(angle);
    transform.position.y = 600 + radius * std::sin(angle); // Starts high, curves down

    if (progress >= 1.0f)
    {
        timeline.IsTransitioningIn = false;
        Framework::engineState.SetPaused(false);
    }
}

void BlinkEffect(Framework::Entity entity, float timer)
{
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float blinkSpeed = 0.5f; // Adjust for faster/slower blinking

    // Calculate blinking alpha using a sine wave for smooth fade
    render.alpha = static_cast<float>(0.5f + 0.5f * std::sin(blinkSpeed * timer * M_PI));

    if (timeline.IsTransitioningIn)
    {
        timeline.InternalTimer += 0.016f; // Assuming 60FPS (frame-based update)

        if (timeline.InternalTimer >= timeline.TransitionDuration)
        {
            GlobalAssetManager.UE_LoadPrefab("BossBar.json");
            timeline.IsTransitioningIn = false; // Stop blinking after duration
            render.alpha = 0.f; // Ensure it stays fully visible at the end
        }
    }
}

void BlinkEffectWithoutBossSpawnPrefabs(Framework::Entity entity, float timer)
{
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    float blinkSpeed = 0.5f; // Adjust for faster/slower blinking

    // Calculate blinking alpha using a sine wave for smooth fade
    render.alpha = static_cast<float>(0.5f + 0.5f * std::sin(blinkSpeed * timer * M_PI));

    if (timeline.IsTransitioningIn)
    {
        timeline.InternalTimer += 0.016f; // Assuming 60FPS (frame-based update)

        if (timeline.InternalTimer >= timeline.TransitionDuration)
        {
            timeline.IsTransitioningIn = false; // Stop blinking after duration
            render.alpha = 0.f; // Ensure it stays fully visible at the end
        }
    }
}


/*  Commented out post merge due to duplicate, do seperate future timeline functions as referred 
to below as we're sharing this cpp and will have a lot of segments.

void FadeInEvent(Framework::Entity entity, float progress) {
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);

    // Reduce alpha value to fade out
    render.alpha = 0.0f + progress;
}*/


void FadeInEvent(Framework::Entity entity, float progress) {
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);

    // Ensure alpha is within valid range (0.0 to 1.0)
    render.alpha = std::clamp(progress, 0.0f, 1.0f);

    if (progress >= 1.0f) {  // Fully faded in
        std::cout << "LOGO COMPLETELY FADED IN" << std::endl;

        // Start counting how long the logo stays on screen
        timeline.InternalTimer = 0.0f;  // Reset the timer

        // Mark the transition phase
        timeline.IsTransitioningIn = false;
    }
}


void FadeOutEvent(Framework::Entity entity, float progress) {
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
    (void)timeline;
    // Ensure alpha smoothly transitions from 1 to 0 over time
    render.alpha = std::clamp(1.0f - progress, 0.0f, 1.0f);

    if (progress >= 1.0f) {
        render.isActive = false;
    }

}

void FadeOutThenTransitMenu(Framework::Entity entity, float progress) {
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);

    // Reduce alpha value to fade out
    //render.alpha = 1.0f - progress;

    // Ensure alpha smoothly transitions from 1 to 0 over time
    render.alpha = std::clamp(1.0f - progress, 0.0f, 1.0f);

    if (progress >= 1.0f) {
        Framework::GlobalSceneManager.TransitionToScene("Assets/Scene/MenuScene.json");
    }
}

void RetryFunction(Framework::Entity entity, float progress) {
    (void)entity;
    (void)progress;

    Framework::GlobalSceneManager.TransitionToScene("Assets/Scene/GameLevel.json");
}

void ScaleUpEvent(Framework::Entity entity, float progress) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    (void)transform;
    (void)progress;
    // Scale up the entity from 0.5x to 1.0x size
  //  float scaleFactor = Lerp(0.5f, 1.0f, progress);
    //transform.scale.x = scaleFactor;
    //transform.scale.y = scaleFactor;

  //  std::cout << "ScaleUpEvent progress: " << progress << std::endl;
}

void TransitionToSceneEvent(Framework::Entity entity, float progress) {
    (void)entity;

    // Trigger scene transition when progress reaches 1.0
    if (progress >= 1.0f) {
        //Framework::GlobalSceneManager.TransitionToScene("Assets/Scene/GameLevel.json");
        //std::cout << "TransitionToSceneEvent triggered!" << std::endl;
        std::cout << "StartScreenAnimation complete! Transitioning to GameLevel.json." << std::endl;
        Framework::GlobalSceneManager.TransitionToScene("Assets/Scene/GameLevel.json");
    }
}

// Text prefab functions 

// Text Pop Up Effect (Font Size-Based Shrinking) zoom in version
void TextPopupFlyOut(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& text = ecsInterface.GetComponent<TextComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);

    // Progress between 0 and 1 (clamped)
    float progress = std::min(timer / timeline.TransitionDuration, 1.0f);

    // **Step 1: Floating Up Effect**
    float floatSpeed = 1.5f; // Adjust for faster/slower floating
    transform.position.y -= floatSpeed * (1.0f - progress); // Moves upward smoothly

    // **Step 2: Decrease Font Size Over Time**
    float startFontSize = text.fontSize; // Original font size
    float endFontSize = (startFontSize * 0.75f); // Reduce to 75% of original
    text.fontSize = (Lerp(static_cast<float>(startFontSize),
        static_cast<float>(endFontSize), progress));

    // **Step 3: Fade Out Effect**
    render.alpha = 1.0f - progress; // Gradual fade-out

    // **Step 4: Cleanup when complete**
    if (progress >= 1.0f) {
        //ecsInterface.DestroyEntity(entity); // Remove entity when animation is done
    }
}

/// Text Pop Up Effect (Font Size-Based Shrinking) zoom in version
void TextPopup(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& text = ecsInterface.GetComponent<TextComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);

    // Progress between 0 and 1 (clamped)
    float progress = std::min(timer / timeline.TransitionDuration, 1.0f);

    // **Step 1: Floating Up Effect**
    float floatSpeed = 1.5f; // Adjust for faster/slower floating
    transform.position.y -= floatSpeed * (1.0f - progress); // Moves upward smoothly

    // **Step 2: Initialize Font Size Variables Only Once**
    if (timeline.Variable_1 == 0.0f) {
        timeline.Variable_1 = text.fontSize;          // Store original font size
        timeline.Variable_2 = text.fontSize * 1.5f;  // Store peak expanded size
        timeline.Variable_3 = text.fontSize;         // Current size (to modify smoothly)
    }

    // **Step 3: Font Expansion and Shrinking Over Duration**
    float scaleProgress = static_cast<float>(sin(progress * 3.125863252431)); // Creates a smooth expand & shrink effect
    timeline.Variable_3 = Lerp(timeline.Variable_1, timeline.Variable_2, scaleProgress);
    text.fontSize = timeline.Variable_3; // Apply calculated font size


    // **Step 5: Cleanup when complete**
    if (progress >= 1.0f) {
        //ecsInterface.DestroyEntity(entity); // Remove entity when animation is done
        render.isActive = false; // Disable rendering instead of deleting
    }
}

// Text prefab functions end 


// ------------------- Ability functions -------------- //

void SlowPrefabFunction(Framework::Entity entity, float timer) {
    auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
    auto& text = ecsInterface.GetComponent<TextComponent>(entity);
    auto& timeline = ecsInterface.GetComponent<TimelineComponent>(entity);
    auto& render = ecsInterface.GetComponent<RenderComponent>(entity);

    (void)transform;
    (void)text;
    // Progress between 0 and 1 (clamped)
    float progress = std::min(timer / timeline.TransitionDuration, 1.0f);

    //Set slowed mode 
    Framework::engineState.isSlow = true;

    //Boolean switch toggle to play once.
    if (!timeline.Variable_2)
    {
        Framework::GlobalAudio.UE_PlaySound("TimeSlow", false);         
        timeline.Variable_2 = 1; // switch
    }

    // **Gradual Slowdown & Alpha Transition**
    if (progress < 0.25f) 
    {
        // First 25%: Gradually slow down from 1.0 to 0.5
        float t = progress / 0.25f; // Normalize 0-0.25 to 0-1
        Framework::engineState.TimeScale = 1.0f - (0.5f * t);

        // Gradually increase alpha from 0 to 0.5
        render.alpha = 0.5f * t;
    }
    else if (progress < 0.75f) 
    {

        // Middle phase: Keep timescale at 0.5
        Framework::engineState.TimeScale = 0.5f;

        // Maintain alpha at 0.5
        render.alpha = 0.5f;
    }
    else 
    {
        //Boolean switch toggle to play once.
        if (!timeline.Variable_1)
        {
            Framework::GlobalAudio.UE_PlaySound("TimeGoBack", false);
            timeline.Variable_1 = 1; // switch
        }
        // Last 25%: Gradually speed back to 1.0 using smoothstep
        float t = (progress - 0.75f) / 0.25f; // Normalize 0.75-1 to 0-1
        t = t * t * (3.0f - 2.0f * t); // Smoothstep function for smoother transition
        Framework::engineState.TimeScale = 0.5f + (0.5f * t);

        // Gradually decrease alpha from 0.5 to 0
        render.alpha = 0.5f * (1.0f - t);
    }

    // **Reset & Cleanup**
    if (progress >= 1.0f) 
    {
        Framework::engineState.TimeScale = 1.0f; // Restore normal speed
        render.alpha = 0.0f; // Fully transparent
        Framework::engineState.isSlow = false; // reset
        render.isActive = false; // Disable rendering instead of deleting

    }
}

void RegisterTimelineEvents() {
    // Register timeline events to the LogicManager // Use this naming for ECS systems to register
    GlobalLogicManager.RegisterTimelineFunction("SlideIn", SlideInTransition);
    GlobalLogicManager.RegisterTimelineFunction("SlideY", SlideUp);
    GlobalLogicManager.RegisterTimelineFunction("CreditsY", Credits);
    GlobalLogicManager.RegisterTimelineFunction("Blinking", BlinkEffect);
    GlobalLogicManager.RegisterTimelineFunction("BlinkingNoSpawn", BlinkEffectWithoutBossSpawnPrefabs);
    GlobalLogicManager.RegisterTimelineFunction("SlideOut", SlideOut);
    GlobalLogicManager.RegisterTimelineFunction("SlideOutWarning", SlideOutWarning);
    GlobalLogicManager.RegisterTimelineFunction("FadeOut", FadeOutEvent);
    GlobalLogicManager.RegisterTimelineFunction("FadeIn", FadeInEvent);
    GlobalLogicManager.RegisterTimelineFunction("ScaleUp", ScaleUpEvent);
    GlobalLogicManager.RegisterTimelineFunction("TransitionToScene", TransitionToSceneEvent);
    GlobalLogicManager.RegisterTimelineFunction("RetryFunctions", RetryFunction);
    GlobalLogicManager.RegisterTimelineFunction("SlideInBounce", SlideInBounce);
    GlobalLogicManager.RegisterTimelineFunction("SlideInWobbly", SlideInWobbly);
    GlobalLogicManager.RegisterTimelineFunction("SlideInCircular", SlideInCircular);
    GlobalLogicManager.RegisterTimelineFunction("SlideInElastic", SlideInElastic);
    GlobalLogicManager.RegisterTimelineFunction("SlideDiag", SlideDiag);

    //Text prefab timelines
    GlobalLogicManager.RegisterTimelineFunction("TextPopUp", TextPopup);
    GlobalLogicManager.RegisterTimelineFunction("TextPopUpFlyOut", TextPopupFlyOut);

    GlobalLogicManager.RegisterTimelineFunction("FadeOutTransitionToMenu", FadeOutThenTransitMenu);

    //Ability prefab functions
    GlobalLogicManager.RegisterTimelineFunction("SlowAbilityPrefab", SlowPrefabFunction);

    std::cout << "Timeline events registered." << std::endl;
}

