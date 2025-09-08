#pragma once
///////////////////////////////////////////////////////////////////////////////
///
///@file AnimationSystem.h
/// 
/// @brief AnimationSystem.h handles the creation of the Animation class and the declaration of its functions and members
///  
///@Authors: Dylan Lau
///Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "System.h"
#include "Vector2D.h"
#include "InputHandler.h"
#include "ComponentList.h"

namespace Framework 
{
    class AnimationSystem : public ISystem 
    {
    public:
        AnimationSystem();
        ~AnimationSystem();

        void Initialize();
        void Update(float deltaTime) override;
        std::string GetName() override;

        /**
        * @brief Run the selected animation for 1 cycle
        *
        * This function takes in the name of 2 sprite sheet. The first sprite sheet will be played once and 2nd will be played all the way.
        *
        * @param render : Render component of the object
        * @param collision : collision component of the object
        * @param animation : animation component of the object
        * @param deltaTime : total time per frame
        * @param rows : the amount of rows in the spritesheet
        * @param cols : the amount of cols in the spritesheet
        * @param animationTime : the total time passed since the animation has changed
        * @param animationPlayed : the name of the sprite sheet that will be played one cycle
        * @param defaultAnimation : the name of the sprite sheet that will be played all the way after the first animation has been played once.
        *
        */

        void UE_CollidedShortAnimation(RenderComponent& render, CollisionComponent& collision, AnimationComponent& animation, float deltaTime, int rows, int cols, float animationTime, std::string animationPlayed, std::string defaultAnimation);
    };
}