#include "../header/Effect.hlsli"

// CBV : EffectRoot
ConstantBuffer<EffectRoot> gEffectRoot : register(b0);
// SRV : EffectNode
StructuredBuffer<EffectNode> gEffectNode : register(t0);
// SRV : EffectMeshData
StructuredBuffer<EffectSprite> gEffectMesh : register(t1);

// UAV : EffectParticle 128x1024個
RWStructuredBuffer<EffectParticle> gEffectParticle : register(u0);
// UAV : EffectParticleFreeList 128x1024個
AppendStructuredBuffer<uint> gEffectParticleFreeList : register(u1);

[numthreads(kMaxParticles, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID) {
    uint particleIndex = DTid.x;
    // パーティクルが死んでいるなら何もしない
    if (gEffectParticle[particleIndex].isAlive == 0) {
        return;
    }
    uint nodeIndex = gEffectParticle[particleIndex].nodeID;
    uint meshIndex = gEffectNode[nodeIndex].draw.meshDataIndex;
    if (gEffectParticle[particleIndex].nodeID != nodeIndex) {
        return;
    }
    // 経過時間が生存時間を超えているか
    if (gEffectParticle[particleIndex].currentTime >= gEffectParticle[particleIndex].lifeTime) {
        // スケールに0を入れておいてVertexShader出力で棄却されるようにする
        gEffectParticle[particleIndex].scale.value = float3(0, 0, 0);
        // パーティクルを死なせる
        gEffectParticle[particleIndex].isAlive = 0;
        gEffectParticle[particleIndex].currentTime = 0;
        gEffectParticle[particleIndex] = (EffectParticle) 0; // パーティクルを初期化
        // フリーリストに追加
        gEffectParticleFreeList.Append(particleIndex);
        return;
    }
    if (gEffectParticle[particleIndex].isAlive) { // 生きているパーティクルの更新
        // 位置
        gEffectParticle[particleIndex].position.value += gEffectParticle[particleIndex].position.velocity * gEffectRoot.timeManager.deltaTime;
        // 速度
        gEffectParticle[particleIndex].position.velocity += gEffectParticle[particleIndex].position.acceleration * gEffectRoot.timeManager.deltaTime;
        // 回転
        gEffectParticle[particleIndex].rotation.value += gEffectParticle[particleIndex].rotation.velocity * gEffectRoot.timeManager.deltaTime;
        // 回転速度
        gEffectParticle[particleIndex].rotation.velocity += gEffectParticle[particleIndex].rotation.acceleration * gEffectRoot.timeManager.deltaTime;
        // スケール
        gEffectParticle[particleIndex].scale.value += gEffectParticle[particleIndex].scale.velocity * gEffectRoot.timeManager.deltaTime;
        // スケール速度
        gEffectParticle[particleIndex].scale.velocity += gEffectParticle[particleIndex].scale.acceleration * gEffectRoot.timeManager.deltaTime;
        // 色
        // 色のタイプごとに色を更新
        switch (gEffectMesh[meshIndex].colorType) {
            case COLOR_TYPE_CONSTANT:
                break;
            case COLOR_TYPE_RANDOM:
                break;
            case COLOR_TYPE_EASING:
                break;
        }
        // 経過時間更新
        gEffectParticle[particleIndex].currentTime++;
    }
}