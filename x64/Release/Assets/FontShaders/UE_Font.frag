///////////////////////////////////////////////////////////////////////////////
///
///	@file UE_Font.frag
/// 
/// @brief Fragment files for font shader
///	
///	@Authors: Joshua Sim Yue Chen
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text; // Texture sampler for the glyph
uniform vec3 textColor; // Text color

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r); // Sample the glyph texture (red channel)
    FragColor = vec4(textColor, 1.0) * sampled; // Apply the text color to the sampled texture
}