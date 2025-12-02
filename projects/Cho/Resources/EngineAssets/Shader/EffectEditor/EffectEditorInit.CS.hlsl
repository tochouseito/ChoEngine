#include "../header/Effect.hlsli"

// UAV : EffectParticle 128x1024個
RWStructuredBuffer<EffectParticle> gEffectParticle : register(u0);
// UAV : EffectParticleFreeList 128x1024個
AppendStructuredBuffer<uint> gEffectParticleFreeList : register(u1);
// UAV : ParticleFreeListCounter 1個
RWStructuredBuffer<uint> gEffectParticleFreeListCounter : register(u2);

[numthreads(kMaxParticles, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID) {
    // すべてのバッファの初期化
    // 逆順にAppend
    uint i = (128 * kMaxParticles) - DTid.x - 1;
    // 各カウンターを一回だけ初期化
    if (i == 0) {
        gEffectParticleFreeListCounter[0] = 0;
    }
    //--Particle--//
    gEffectParticle[i] = (EffectParticle) 0;
    gEffectParticleFreeList.Append(i);
}