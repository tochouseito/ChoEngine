#pragma once
#include "EffectMeshCommon.hlsli"

static const uint RING_SHAPETYPE_DONUT = 0;
static const uint RING_SHAPETYPE_CRESCENT = 1;

struct EffectRing
{
    uint shape;
    float startFade;
    float endFade;
    float startAngle;
    float endAngle;
    uint placement;
    uint vertexCount;
    float2 outerRing;
    float2 innerRing;
    float centerRatio;
    float4 outerRingColor;
    float4 centerColor;
    float4 innerRingColor;
};