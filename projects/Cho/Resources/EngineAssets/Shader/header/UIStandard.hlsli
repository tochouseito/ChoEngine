#pragma once


struct World
{
    float4x4 World;
    float left; 
    float right; 
    float top; 
    float bottom; 
    float tex_left; 
    float tex_right; 
    float tex_top;
    float tex_bottom;
    uint materialID;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    uint materialID : MATERIALID0;
};