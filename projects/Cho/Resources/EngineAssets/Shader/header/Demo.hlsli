#pragma once

struct Transform
{
    float4x4 matWorld;
    float4x4 worldInverse;
    float4x4 rootNode;
    uint materialID;
    uint isAnimated;
    uint isBoneOffsetStartIndex;
    float padding[1];
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float3 cameraPosition : CAMERAPOS0;
};

struct VSOut 
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
    float3 cameraPosition : CAMERAPOS0;
    uint materialID : MATERIALID0;
    float4 color : COLOR0;
};

struct ViewProjection
{
    float4x4 view;
    float4x4 projection;
    float4x4 projectionInverse;
    float4x4 matWorld;
    float4x4 matBillboard;
    float3 cameraPosition;
};