#pragma once

static const uint MAX_LIGHTS = 30;
static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
struct Light {
    float4 color;
    float3 direction;
    float intensity;
    float range;
    float decay;
    float spotAngle;
    float spotFalloffStart;
    uint type;
    uint active;
    uint transformMapID;
};
struct Lights {
    Light lights[MAX_LIGHTS];
};