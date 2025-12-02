#include "../header/EffectRoot.hlsli"
#include "../header/EffectNode.hlsli"
#include "../header/EffectParticle.hlsli"
#include "../header/Random.hlsli"

// SRV : Root 128個
StructuredBuffer<EffectRoot> gRoots : register(t0);
// SRV : Node 1024個
StructuredBuffer<EffectNode> gNodes : register(t1);
// SRV : Sprite 1024個
StructuredBuffer<EffectSprite> gSprites : register(t2);
// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0);
// UAV : ParticleFreeList 128x1024個
AppendStructuredBuffer<uint> gParticleFreeList : register(u1);
//-----------------------//
// 他のメッシュ情報を追加予定 //
//-----------------------//

[numthreads(kMaxParticles, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
    uint particleIndex = DTid.x;
    // Particle取得
    EffectParticle particle = gParticles[particleIndex];
    // パーティクルが死んでいるなら何もしない
    if (particle.isAlive == 0) {
        return;
    }
    // Root,Node取得
    EffectRoot root = gRoots[particle.rootID];
    EffectNode node = gNodes[particle.nodeID];
    // 現在時間を取得
    float elapsed = root.time.elapsedTime;
    // デルタタイムを取得
    float delta = root.time.deltaTime;
    // SpawnTimeとelapsedTimeからパーティクルの経過時間を計算
    float currentTime = elapsed - particle.spawnTime;
    // 経過時間が生存時間を超えているか
    if (currentTime >= particle.lifeTime) {
        // スケールに0を入れておいてVertexShader出力で棄却されるようにする
        particle.scale.value = float3(0, 0, 0);
        // パーティクルを死なせる
        particle.isAlive = 0;
        currentTime = 0;
        particle = (EffectParticle) 0; // パーティクルを初期化
        // フリーリストに追加
        gParticleFreeList.Append(particleIndex);
        return;
    }
    // 生きているパーティクルの更新
    if (particle.isAlive) { 
        // 位置
        particle.wPos = particle.position.value + particle.position.velocity * currentTime + 0.5f * particle.position.acceleration * currentTime * currentTime;
        // 回転
        particle.wRot = particle.rotation.value + particle.rotation.velocity * currentTime + 0.5f * particle.rotation.acceleration * currentTime * currentTime;
        // スケール
        particle.wScl = particle.scale.value + particle.scale.velocity * currentTime + 0.5f * particle.scale.acceleration * currentTime * currentTime;
        // 色
        switch (node.draw.meshType) {
            case MESH_TYPE_SPRITE:
                    {
                    EffectSprite sprite = gSprites[particle.meshID];
                    switch (sprite.colorType) {
                        case COLOR_TYPE_CONSTANT:
                            particle.color = sprite.color;
                            break;
                        case COLOR_TYPE_RANDOM:
                            break;
                        case COLOR_TYPE_EASING:
                            break;
                    }
                    break;
                }
            default:
                    // その他のメッシュタイプは未対応
                particle.color = float4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
        }
    }
    // 更新したパーティクルを保存
    gParticles[particleIndex] = particle;
}