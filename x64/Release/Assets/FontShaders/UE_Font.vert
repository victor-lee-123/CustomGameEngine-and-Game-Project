///////////////////////////////////////////////////////////////////////////////
///
///	@file UE.frag
/// 
/// @brief Vert files for font shader
///	
///	@Authors: Joshua sim yue chen
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#version 450 core

layout(location = 0) in vec4 vertex; // (x, y, z, w) and texture coordinates (x, y)
out vec2 TexCoords;

uniform mat4 projection; // Projection matrix for rendering text

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0); // Set position using the projection matrix
    TexCoords = vertex.zw; // Pass texture coordinates to the fragment shader
}