#pragma once

struct Transform
{
    float4x4 worldMatrix;
};

struct ViewProjection
{
    float4x4 view;
    float4x4 projection;
};

struct ModelInfo
{
    uint indexOffset;
    uint indexCount;
    int baseVertex;
    uint pad;
};

struct VSIn
{
    float4 position : POSITION;
};

struct VSOut
{
    float4 position : SV_POSITION;
};
