#pragma once

struct VSOutput
{
    float4 position : SV_POSITION; // Position in clip space
    float3 texCoord : TEXCOORD0; // Texture coordinates for the skybox
};
struct SkyboxTransform
{
    float4x4 worldMatrix; // World matrix for the skybox
};