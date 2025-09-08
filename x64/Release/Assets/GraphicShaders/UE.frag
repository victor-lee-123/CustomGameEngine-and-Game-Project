///////////////////////////////////////////////////////////////////////////////
///
///	@file UE.frag
/// 
/// @brief Fragment files for shader
///	
///	@Authors: Victor lee
///	Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////



#version 450 core

// Interpolated texture coordinates from the vertex shader
layout (location=0) in vec2 vTexCoord;

// Output fragment color
layout (location=0) out vec4 fFragColor;

// Texture sampler
uniform sampler2D uTexture;

// Color uniform
uniform vec3 uColor;

// Alpha uniform 
uniform float uAlpha;  // Value between 0.0 (fully transparent) and 1.0 (fully opaque)

// Use texture flag
uniform bool useTexture;  // If true, use the texture; otherwise, use only the color

void main()
{
    vec4 texColor = vec4(uColor, uAlpha);  // Default to color

    // If we're using a texture, sample the texture color
    if (useTexture)
    {
        texColor = texture(uTexture, vTexCoord) * vec4(uColor, uAlpha);
    }

    // Final fragment color
    fFragColor = texColor;
}
