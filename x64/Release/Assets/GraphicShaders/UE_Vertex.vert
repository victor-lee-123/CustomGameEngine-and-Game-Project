///////////////////////////////////////////////////////////////////////////////
///
/// @file UE_texCoord.vert
/// 
/// @brief Vertex shader for passing texture coordinates.
///	
/// @Authors: Victor Lee
/// Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#version 450 core

layout(location = 1) in vec2 texCoord;   // Texture coordinates input

layout(location = 0) out vec2 TexCoord;  // Pass texture coordinates to the fragment shader

void main()
{
    // Directly pass texture coordinates to the fragment shader
    TexCoord = texCoord;
}
