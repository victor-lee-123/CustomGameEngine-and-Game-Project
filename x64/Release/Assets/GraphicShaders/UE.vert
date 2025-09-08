///////////////////////////////////////////////////////////////////////////////
///
///	@file UE.vert
/// 
/// @brief Fragment files for shader
///	
///	@Authors: Victor lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////



#version 450 core

layout(location = 0) in vec2 position; // Vertex position in object space
layout(location = 1) in vec2 texCoord; // Texture coordinates

// Transformation matrices
uniform mat4 modelMatrix;        // Local object transformations
uniform mat4 viewMatrix;         // View transformation (from camera)
uniform mat4 projectionMatrix;   // Projection transformation (usually orthographic for 2D)

layout(location = 0) out vec2 TexCoord; // Pass texture coordinates to the fragment shader

void main()
{
    // Calculate final vertex position in clip space
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 0.0, 1.0);
    TexCoord = texCoord;
}

