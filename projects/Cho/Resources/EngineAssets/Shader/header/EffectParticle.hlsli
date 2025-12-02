#pragma once

struct EffectParticlePVA
{
    float3 value;
    float3 velocity;
    float3 acceleration;
};
struct EffectParticle
{
    float3 wPos;
    float3 wRot;
    float3 wScl;
    EffectParticlePVA position;
    EffectParticlePVA rotation;
    EffectParticlePVA scale;
    float4 color;
    float lifeTime;
    float spawnTime;
    uint isAlive;
    uint rootID;
    uint nodeID;
    uint meshType;
    uint meshID;
};
static const uint kMaxParticles = 1024;