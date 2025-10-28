#pragma once

struct RandValue
{
    float median;
    float amplitude;
};

struct Randfloat3
{
    RandValue x;
    RandValue y;
    RandValue z;
};

struct PVA
{
    Randfloat3 value;
    Randfloat3 velocity;
    Randfloat3 acceleration;
};

struct EasingValue
{
    Randfloat3 startPoint;
    Randfloat3 endPoint;
    uint easingType;
    uint startSpeedType;
    uint endSpeedType;
    uint isMedianPoint;
    Randfloat3 medianPoint;
};

static const uint SRT_TYPE_STANDARD = 0;
static const uint SRT_TYPE_PVA = 1;
static const uint SRT_TYPE_EASING = 2;

struct EffectSRT
{
    uint type;
    float3 value;
    PVA pva;
    EasingValue easing;
};