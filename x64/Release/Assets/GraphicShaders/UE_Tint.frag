///////////////////////////////////////////////////////////////////////////////
///
///	@file UE_color.frag
/// 
/// @brief Fragment shader for handling solid colors
///	
///	@Authors: Victor Lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#version 450 core

uniform int entityID; // Pass the unique entity ID from your application
out vec4 FragColor;

void main()
{
    // Encode entity ID into an RGB color
    FragColor = vec4(
        float((entityID & 0x000000FF)) / 255.0,
        float((entityID & 0x0000FF00) >> 8) / 255.0,
        float((entityID & 0x00FF0000) >> 16) / 255.0,
        1.0);
}
