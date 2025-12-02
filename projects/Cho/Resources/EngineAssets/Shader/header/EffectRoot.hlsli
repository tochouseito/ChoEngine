#pragma once

struct TimeManager
{
    float elapsedTime;
    float duration;
    float deltaTime;
    float padding;
};

struct EffectRoot
{
    TimeManager time;
    uint globalSeed;
    uint nodeCount;
    uint4 nodeIDs[32];
};
static const uint kMaxRoot = 128;

uint FetchEffectNodeID(EffectRoot root, uint idx)
{
    uint row = idx >> 2; // idx / 4
    uint col = idx & 3; // idx % 4
    return root.nodeIDs[row][col];
}

struct VSInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
    
};
struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    uint instanceId : SV_InstanceID;
};