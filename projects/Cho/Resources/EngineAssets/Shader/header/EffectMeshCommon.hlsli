#pragma once

static const uint MESH_TYPE_NONE = 0;
static const uint MESH_TYPE_SPRITE = 1;
static const uint MESH_TYPE_RIBBON = 2;
static const uint MESH_TYPE_TRAIL = 3;
static const uint MESH_TYPE_RING = 4;
static const uint MESH_TYPE_MODEL = 5;
static const uint MESH_TYPE_CUBE = 6;
static const uint MESH_TYPE_SPHERE = 7;

static const uint COLOR_TYPE_CONSTANT = 0;
static const uint COLOR_TYPE_RANDOM = 1;
static const uint COLOR_TYPE_EASING = 2;

struct RandColor
{
    float4 minColor;
    float4 maxColor;
};

struct EasingColor
{
    float4 startMinColor;
    float4 startMaxColor;
    float4 endMinColor;
    float4 endMaxColor;
    uint startSpeedType;
    uint endSpeedType;
};

static const uint PLACEMENT_TYPE_BILLBOARD = 0;
static const uint PLACEMENT_TYPE_BILLBOARDY = 1;
static const uint PLACEMENT_TYPE_BILLBOARDXY = 2;
static const uint PLACEMENT_TYPE_CONSTANT = 3;

static const uint VERTEX_COLOR_TYPE_STANDARD = 0;
static const uint VERTEX_COLOR_TYPE_CONSTANT = 1;

static const uint VERTEX_TYPE_STANDARD = 0;
static const uint VERTEX_TYPE_CONSTANT = 1;

struct EffectDraw
{
    uint meshType;
    uint meshID;
};