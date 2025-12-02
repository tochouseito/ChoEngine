#pragma once

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};
static const uint kNumVertex = 3;
static const float4 kPositions[kNumVertex] =
{
    { -1.0f, 1.0f, 0.0f, 1.0f }, // left top
    { 3.0f, 1.0f, 0.0f, 1.0f }, // right top
    { -1.0f, -3.0f, 0.0f, 1.0f }, // left bottom
};
static const float2 kTexcoords[kNumVertex] =
{
    { 0.0f, 0.0f }, // left top
    { 2.0f, 0.0f }, // right top
    { 0.0f, 2.0f }, // left bottom
};
struct PSOutput
{
    float4 color : SV_TARGET0;
};