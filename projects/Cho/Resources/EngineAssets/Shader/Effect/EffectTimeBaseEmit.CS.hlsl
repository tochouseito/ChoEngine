#include "../header/EffectRoot.hlsli"
#include "../header/EffectNode.hlsli"
#include "../header/EffectParticle.hlsli"
#include "../header/Random.hlsli"

// SRV : UseRootNumber 128個
StructuredBuffer<uint> gUseRootIDs : register(t0);// 有効なRootのID
// SRV : Root 128個
StructuredBuffer<EffectRoot> gRoots : register(t1);
// SRV : Node 1024個
StructuredBuffer<EffectNode> gNodes : register(t2);
// SRV : Sprite 1024個
StructuredBuffer<EffectSprite> gSprites : register(t3);
// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0);
// UAV : ParticleFreeList 128x1024個
ConsumeStructuredBuffer<uint> gParticleFreeList : register(u1);
// UAV : ParticleFreeListCounter 1個
RWStructuredBuffer<uint> gParticleCounter : register(u2);
//-----------------------//
// 他のメッシュ情報を追加予定 //
//-----------------------//

// 現フレームで発生すべきパーティクル数
uint CalcSpawnCount(
    in EffectCommon common,
    float elapsed,
    //in RandomGenerator generator,
    inout float spawnTime)
{
    // 発生開始時間を設定
    //float emitStartTime = GenerateRandomInRange(generator.Generate1d(), common.emitStartTime.median, common.emitStartTime.amplitude);
    float emitStartTime = common.emitStartTime.median;
    // 発生開始前なら 0
    if (elapsed < emitStartTime)
    {
        return 0;
    }

    // 経過時間からサイクル数を計算
    float t = elapsed - emitStartTime;
    uint totalAllowed;
    // 発生間隔を設定
    
    //float emitInterval = GenerateRandomInRange(generator.Generate1d(), common.emitInterval.median, common.emitInterval.amplitude);
    float emitInterval = common.emitInterval.median;
    if (emitInterval <= 0.0f)
    {
        // 間隔ゼロ=瞬時に一度だけ emitCount 発生
        totalAllowed = common.emitCount;
    }
    else
    {
        // 第何サイクル目か（0-based）を floor で取得し +1
        float cycle = t / emitInterval;
        uint numCycles = (uint) floor(cycle) + 1;
        totalAllowed = numCycles * common.emitCount;
    }

    // 上限をかける
    totalAllowed = min(totalAllowed, common.emitCountMax);
    
    // 発生時間を算出
    uint cycleIndex = (totalAllowed - 1) / common.emitCount;
    float cycleTime = emitStartTime + cycleIndex * emitInterval;
    spawnTime = cycleTime;

    return totalAllowed;
}

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 Gid : SV_GroupID) {
    uint rootIdx = DTid.x;
    if (rootIdx >= kMaxRoot)
        return;
    
    // 有効なRootID
    uint rootID = gUseRootIDs[rootIdx];
    //if (rootID == UINT_MAX) return;

    // Root取得
    EffectRoot root = gRoots[rootID];
    // 時間情報を取得
    float elapsed = root.time.elapsedTime;
    float delta = root.time.deltaTime;
    // Rootに紐づくNodeをループ
    for (uint i = 0; i < root.nodeCount; ++i) {
        // Node取得
        uint nodeID = FetchEffectNodeID(root, i);
        EffectNode node = gNodes[nodeID];
        // 生成するパーティクル数を計算
        uint spawnCount = 0; // 生成するパーティクル数
        uint emitCount = 0; // 発生回数
        if (elapsed >= node.common.emitStartTime.median)
        {
            // 発生回数
            // 間隔が0なら一回の発生ですべてのパーティクルを生成
            if (node.common.emitInterval.median != 0.0f)
            {
                uint inv = 1 / node.common.emitInterval.median;
                emitCount = (elapsed - node.common.emitStartTime.median) * inv + 1; // 発生回数は 0 から始まるので +1
                // 同時発生数と発生回数から生成するパーティクル数を計算
                spawnCount = node.common.emitCount * emitCount;
            }
            else
            {
                emitCount = 1;
                spawnCount = node.common.emitCountMax; // 間隔が0なら一度に最大数を生成
            }
        }
        else
        {
            // 発生開始時間を超えていなければ0
            spawnCount = 0;
        }
        // 生成するパーティクル数が 0 ならスキップ
        if (spawnCount == 0)
        {
            continue;
        }
        // 生成するパーティクルの数が最大数より多いなら範囲内に収める
        if (spawnCount > node.common.emitCountMax)
        {
            spawnCount = node.common.emitCountMax;
        }
        // 生成
        for (uint pi = 0; pi < spawnCount; ++pi) {
            // パーティクルインデックスを取得
            if (gParticleCounter[0] == 0) {
                break; // 空き無し
            }
            uint particleIdx = gParticleFreeList.Consume();
            // パーティクル初期化
            EffectParticle particle = gParticles[particleIdx];
            // シード生成
            RandomGenerator generator;
            generator.seed = (pi + root.globalSeed) * root.globalSeed;
            // Position
            switch (node.position.type) {
                case SRT_TYPE_STANDARD:
                    particle.position.value = node.position.value;
                    break;
                case SRT_TYPE_PVA:
                    particle.position.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.value.x.median, node.position.pva.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.value.y.median, node.position.pva.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.value.z.median, node.position.pva.value.z.amplitude)
                        );
                    particle.position.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.velocity.x.median, node.position.pva.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.velocity.y.median, node.position.pva.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.velocity.z.median, node.position.pva.velocity.z.amplitude)
                        );
                    particle.position.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.acceleration.x.median, node.position.pva.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.acceleration.y.median, node.position.pva.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.position.pva.acceleration.z.median, node.position.pva.acceleration.z.amplitude)
                        );
                    break;
                case SRT_TYPE_EASING:
                    break;
            }
            // Rotation
            switch (node.rotation.type) {
                case SRT_TYPE_STANDARD:
                    particle.rotation.value = node.rotation.value;
                    break;
                case SRT_TYPE_PVA:
                    particle.rotation.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.value.x.median, node.rotation.pva.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.value.y.median, node.rotation.pva.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.value.z.median, node.rotation.pva.value.z.amplitude)
                        );
                    particle.rotation.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.velocity.x.median, node.rotation.pva.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.velocity.y.median, node.rotation.pva.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.velocity.z.median, node.rotation.pva.velocity.z.amplitude)
                        );
                    particle.rotation.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.acceleration.x.median, node.rotation.pva.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.acceleration.y.median, node.rotation.pva.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.rotation.pva.acceleration.z.median, node.rotation.pva.acceleration.z.amplitude)
                        );
                    break;
                case SRT_TYPE_EASING:
                    break;
            }
            // Scale
            switch (node.scale.type) {
                case SRT_TYPE_STANDARD:
                    particle.scale.value = node.scale.value;
                    break;
                case SRT_TYPE_PVA:
                    particle.scale.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.value.x.median, node.scale.pva.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.value.y.median, node.scale.pva.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.value.z.median, node.scale.pva.value.z.amplitude)
                        );
                    particle.scale.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.velocity.x.median, node.scale.pva.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.velocity.y.median, node.scale.pva.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.velocity.z.median, node.scale.pva.velocity.z.amplitude)
                        );
                    particle.scale.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.acceleration.x.median, node.scale.pva.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.acceleration.y.median, node.scale.pva.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), node.scale.pva.acceleration.z.median, node.scale.pva.acceleration.z.amplitude)
                        );
                    break;
                case SRT_TYPE_EASING:
                    break;
            }
            // Color
            switch (node.draw.meshType) {
                case MESH_TYPE_SPRITE:
                    {
                        EffectSprite sprite = gSprites[node.draw.meshID];
                        switch (sprite.colorType) {
                            case COLOR_TYPE_CONSTANT:
                                particle.color = sprite.color;
                                break;
                            case COLOR_TYPE_RANDOM:
                                particle.color = float4(
                            generator.GenerateInRange(sprite.randColor.minColor.r, sprite.randColor.maxColor.r),
                            generator.GenerateInRange(sprite.randColor.minColor.g, sprite.randColor.maxColor.g),
                            generator.GenerateInRange(sprite.randColor.minColor.b, sprite.randColor.maxColor.b),
                            generator.GenerateInRange(sprite.randColor.minColor.a, sprite.randColor.maxColor.a)
                        );
                                break;
                            case COLOR_TYPE_EASING:
                                particle.color = float4(
                            generator.GenerateInRange(sprite.easingColor.startMinColor.r, sprite.easingColor.startMaxColor.r),
                            generator.GenerateInRange(sprite.easingColor.startMinColor.g, sprite.easingColor.startMaxColor.g),
                            generator.GenerateInRange(sprite.easingColor.startMinColor.b, sprite.easingColor.startMaxColor.b),
                            generator.GenerateInRange(sprite.easingColor.startMinColor.a, sprite.easingColor.startMaxColor.a)
                        );
                                break;
                        }
                        break;
                    }
                default:
                    // その他のメッシュタイプは未対応
                    particle.color = float4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
            }
                // LifeTime
            particle.lifeTime = GenerateRandomInRange(generator.Generate1d(), node.common.lifeTime.median, node.common.lifeTime.amplitude);
            particle.isAlive = 1;
            // スポーンした時間を設定
            float spawnTime = 0.0f;
            uint emitIndex = pi / node.common.emitCount;
            spawnTime = node.common.emitStartTime.median + emitIndex * node.common.emitInterval.median;
            particle.spawnTime = spawnTime;
            particle.rootID = rootID;
            particle.nodeID = nodeID;
            particle.meshType = node.draw.meshType;
            particle.meshID = node.draw.meshID;
            // パーティクルを保存
            gParticles[particleIdx] = particle;
        }

    }

}