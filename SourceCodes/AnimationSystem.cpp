///////////////////////////////////////////////////////////////////////////////
///
///@file AnimationSystem.cpp
/// 
///@brief AnimationSystem.cpp handles animating game entities by cycling through frames of images over time. 
///  
///  
///@Authors: Dylan Lau
///Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include "AnimationSystem.h"
#include <iostream>
#include "Coordinator.h"
#include "InputHandler.h"
#include "FontSystem.h" 
#include "EngineState.h"
#include "Graphics.h"


extern Framework::Coordinator ecsInterface;


namespace Framework {
   
    // Constructor
    AnimationSystem::AnimationSystem()
    {

    }

    // Destructor
    AnimationSystem::~AnimationSystem() {

    }

    // Initialize the system
    void AnimationSystem::Initialize() 
    {
        ecsInterface.RegisterComponent<AnimationComponent>();
        Signature signature;
        signature.set(ecsInterface.GetComponentType<AnimationComponent>());
        ecsInterface.SetSystemSignature<AnimationSystem>(signature);
        std::cout << "Signature for Animation system is: " << signature << std::endl;
    }

    // Update the system
    void AnimationSystem::Update(float deltaTime) 
    {
        if (engineState.IsPaused()) {
            return;
        }
        // Don't run when paused
        
        std::unordered_map<std::string, EntityAsset::Animation>& animations = Framework::GlobalAssetManager.GetAnimationDataMap();

        for (auto const& entityId : mEntities) 
        {    
            if (ecsInterface.HasComponent<RenderComponent>(entityId)) 
            {
                RenderComponent& render = ecsInterface.GetComponent<RenderComponent>(entityId);
                AnimationComponent& animation = ecsInterface.GetComponent<AnimationComponent>(entityId);
                animation.cols = animations[render.textureID].cols;
                animation.rows = animations[render.textureID].rows;
                animation.animationSpeed = animations[render.textureID].animationSpeed;

                if (ecsInterface.HasComponent<CollisionComponent>(entityId)) 
                {
                    CollisionComponent& collision = ecsInterface.GetComponent<CollisionComponent>(entityId);
                    if (ecsInterface.HasComponent<EnemyComponent>(entityId)) 
                    {
                        EnemyComponent& entityStats = ecsInterface.GetComponent<EnemyComponent>(entityId); 
                        if (entityStats.type == Poison) 
                        {
                           UE_CollidedShortAnimation(render, collision, animation, deltaTime, animation.rows, animation.cols, animation.animationTimePlay, "PoisonDamagedIdleSprite", "PoisonDamagedIdleSprite");
                        }
                        if (entityStats.type == Boss) {
                            UE_CollidedShortAnimation(render, collision, animation, deltaTime, animation.rows, animation.cols, animation.animationTimePlay, "BossDamage", "BossIdle");
                        }
                    }
                    if (ecsInterface.HasComponent<PlayerComponent>(entityId)) {
                        PlayerComponent& player = ecsInterface.GetComponent<PlayerComponent>(entityId);
                        if (player.type == Player) 
                        {
                            if (player.health == 0) 
                            {
                               
                                UE_CollidedShortAnimation(render, collision, animation, deltaTime, animation.rows, animation.cols, animation.animationTimePlay, "McDieSprite", "dead");
                            }
                            else 
                            {
                                UE_CollidedShortAnimation(render, collision, animation, deltaTime, animation.rows, animation.cols, animation.animationTimePlay, "McDamagedSprite", "McIdleSprite");
                            }
                        }
                    }
                }
            }  
        } 
    }

    // Get the name of the system
    std::string AnimationSystem::GetName() {
        return "Animation System";
    }

    void AnimationSystem::UE_CollidedShortAnimation(RenderComponent& render, CollisionComponent& collision, AnimationComponent& animation, float deltaTime, int rows, int cols, float animationTime, std::string animationPlayed, std::string defaultAnimation) {
        (void)rows, cols, animationTime;
        if (collision.collided) 
        {
            render.textureID = animationPlayed;
        }
        if (render.textureID == animationPlayed) 
        {
            int totalFrames = animation.cols * animation.rows;
            (void)totalFrames;
            animation.animationTimePlay = animation.animationTimePlay + deltaTime;
            animation.currentFrame = (int)(animation.animationTimePlay * animation.animationSpeed) % (animation.rows * animation.cols);
            if (animation.currentFrame >= ((animation.rows * animation.cols) - 1)) 
            {
                render.textureID = defaultAnimation;
                animation.currentFrame = 0;
                animation.animationTimePlay = 0;
            }
        }
    }
}