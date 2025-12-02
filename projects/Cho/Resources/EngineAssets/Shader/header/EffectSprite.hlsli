#pragma once
#include "EffectMeshCommon.hlsli"

struct SpriteVertexColor
{
    float4 leftBottom;
    float4 rightBottom;
    float4 leftTop;
    float4 rightTop;
};

struct SpriteVertexPosition
{
    float2 leftBottom;
    float2 rightBottom;
    float2 leftTop;
    float2 rightTop;
};

struct EffectSprite
{
    uint colorType;
    float4 color;
    RandColor randColor;
    EasingColor easingColor;
    uint placement;
    uint VertexColorType;
    SpriteVertexColor vertexColor;
    uint VertexPositionType;
    SpriteVertexPosition vertexPosition;
};
static const uint kMaxSpriteDataCount = 1024;