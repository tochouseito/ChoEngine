#pragma once

struct Material
{
    float4 color;
    float4x4 matUV;
    float shininess;
    uint enableLighting;
    uint enableTexture;
    uint textureID;
    uint uvFlipY;
    uint cubeTextureID;
    float cubeUVScale;
};