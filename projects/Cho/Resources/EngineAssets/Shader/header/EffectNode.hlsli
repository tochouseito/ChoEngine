#pragma once
#include "EasingType.hlsli"
#include "EffectSRT.hlsli"
#include "EffectSprite.hlsli"
#include "EffectRing.hlsli"

struct EffectCommon
{
    uint emitCountMax;
    uint emitCount;
    uint isUnlimit;
    uint PosInfluenceType;
    uint RotInfluenceType;
    uint SclInfluenceType;
    uint deleteLifetime;
    uint deleteParentDeleted;
    uint deleteAllChildrenDeleted;
    RandValue lifeTime;
    RandValue emitInterval;
    RandValue emitStartTime;
};

static const uint FADEOUT_TYPE_NONE = 0;
static const uint FADEOUT_TYPE_ONLIFETIME = 1;
static const uint FADEOUT_TYPE_DELETED = 2;

static const uint UV_TYPE_STANDERD = 0;
static const uint UV_TYPE_CONSTANT = 1;
static const uint UV_TYPE_ANIMATION = 2;
static const uint UV_TYPE_SCROLL = 3;

struct UVConstantValue
{
    float2 startPoint;
    float2 scale;
};

struct UVAnimationParameter
{
    float2 startPoint;
    float2 scale;
    uint oneTime;
    uint widthSheetCount;
    uint heightSheetCount;
    uint isLoop;
    RandValue startSheet;
    uint interpolation;
};

struct EffectDrawCommon
{
    uint materialType;
    uint textureID;
    float emissive;
    uint isFadeIn;
    uint isFadeOutType;
    UVConstantValue constantValue;
    UVAnimationParameter animationParameter;
};

struct EffectNode
{
    EffectCommon common;
    EffectSRT position;
    EffectSRT rotation;
    EffectSRT scale;
    EffectDrawCommon drawCommon;
    EffectDraw draw;
    uint isRootParent;
    uint parentNodeID;
};
static const uint kMaxNode = 1024;