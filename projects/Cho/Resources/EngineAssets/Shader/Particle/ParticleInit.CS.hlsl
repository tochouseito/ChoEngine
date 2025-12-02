#include "../header/Particle.hlsli"

static const uint kMaxParticles = 1024;

// UAV: パーティクルリソース
RWStructuredBuffer<Particle> gParticles : register(u0);
// UAV: パーティクルのフリースロットリスト
AppendStructuredBuffer<uint> gFreeList : register(u1);
// UAV: カウンター
RWStructuredBuffer<uint> gCounter : register(u2);

[numthreads(kMaxParticles, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    // 逆順にAppend
    uint particleIndex = kMaxParticles - DTid.x - 1;
    // 最初に一回だけカウンターを初期化
    if (DTid.x == 0) {
        gCounter[0] = 0;
    }
    if (particleIndex < kMaxParticles) {
        gParticles[particleIndex] = (Particle) 0;
        gFreeList.Append(particleIndex);
    }
}
