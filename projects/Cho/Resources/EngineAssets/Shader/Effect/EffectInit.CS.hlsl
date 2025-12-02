#include "../header/EffectParticle.hlsli"

// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0);
// UAV : ParticleFreeList 128x1024個
AppendStructuredBuffer<uint> gParticleFreeList : register(u1);
// UAV : ParticleFreeListCounter 1個
RWStructuredBuffer<uint> gParticleCounter : register(u2);

[numthreads(kMaxParticles, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID) {
    // すべてのバッファの初期化
    // 逆順にAppend
    uint i = (128 * kMaxParticles) - DTid.x - 1;
    // 各カウンターを一回だけ初期化
    if (i == 0) {
        gParticleCounter[0] = 0;
    }
    //--Particle--//
    gParticles[i] = (EffectParticle) 0;
    gParticleFreeList.Append(i);
}