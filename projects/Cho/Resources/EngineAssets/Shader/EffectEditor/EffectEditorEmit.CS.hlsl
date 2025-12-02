#include "../header/Effect.hlsli"
#include "../header/Random.hlsli"

// CBV : EffectRoot
ConstantBuffer<EffectRoot> gEffectRoot : register(b0);
// SRV : EffectNode
StructuredBuffer<EffectNode> gEffectNode : register(t0);
// SRV : EffectMeshData
StructuredBuffer<EffectSprite> gEffectMesh : register(t1);

// UAV : EffectParticle 128x1024個
RWStructuredBuffer<EffectParticle> gEffectParticle : register(u0);
// UAV : EffectParticleFreeList 128x1024個
ConsumeStructuredBuffer<uint> gEffectParticleFreeList : register(u1);
// UAV : ParticleFreeListCounter 1個
RWStructuredBuffer<uint> gEffectParticleFreeListCounter : register(u2);

// UAV : EffectParticleEmitCounter 128x1024個
//RWStructuredBuffer<uint> gEffectParticleEmitCounter : register(u3);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID,uint3 Gid : SV_GroupID) {
    //// シード生成
    //RandomGenerator generator;
    //generator.seed = (DTid + gEffectRoot.timeManager.globalTime) * gEffectRoot.timeManager.globalTime;
    //// 処理対象のNodeのインデックスを取得
    //EffectRoot root = gEffectRoot;
    //uint nodeIndex = FetchEffectNodeID(root,DTid.x);
    //uint meshIndex = gEffectNode[nodeIndex].draw.meshDataIndex;
    //// 発生処理
    //int emitCount = 0; // 発生開始から、発生の間隔時間
    //// 発生開始時間を設定
    //float emitStartTime = GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].common.emitStartTime.median, gEffectNode[nodeIndex].common.emitStartTime.amplitude);
    //// 経過時間が発生開始時間を超えたら発生開始
    //if (gEffectRoot.timeManager.globalTime >= emitStartTime) {
    //    emitCount = gEffectRoot.timeManager.globalTime - emitStartTime;
    //    uint isEmit; //　発生フラグ
    //    if (emitCount == 0) {
    //        isEmit = 1;
    //    }
    //    else {
    //        float emitTime = GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].common.emitTime.median, gEffectNode[nodeIndex].common.emitTime.amplitude);
    //        if (emitTime == 0) {
    //            isEmit = 1; // 発生間隔が0なら発生
    //        }
    //        else {
    //            isEmit = emitCount % emitTime;
    //            if (isEmit == 0) {
    //                isEmit = 1; 
    //            }
    //            else {
    //                isEmit = 0;
    //            }
    //        }
    //    }
    //    if (isEmit != 0) {
    //        // 発生
    //        //// カウンターが発生数を超えたら発生しない
    //        //if (gEffectNode[nodeIndex].emitCounter >= gEffectNode[nodeIndex].emitCounter)
    //        //{
    //        //    return;
    //        //}
    //        for (uint countIndex = 0; countIndex < gEffectNode[nodeIndex].common.emitCount; countIndex++) {
    //            // パーティクルごとにシードを変える
    //            generator.seed = generator.Generate3d() + float3(countIndex, countIndex * 2, countIndex * 3);
    //            // パーティクルインデックスを取得
    //            if (gEffectParticleFreeListCounter[0] == 0) {
    //                break; // 空きなし
    //            }
    //            uint particleIndex = gEffectParticleFreeList.Consume();
    //            // パーティクルの初期化
    //            // Position
    //            switch (gEffectNode[nodeIndex].position.type) {
    //                case SRT_TYPE_STANDARD:
    //                    gEffectParticle[particleIndex].position.value = gEffectNode[nodeIndex].position.value;
    //                    break;
    //                case SRT_TYPE_PVA:
    //                    gEffectParticle[particleIndex].position.value = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.value.x.median, gEffectNode[nodeIndex].position.pva.value.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.value.y.median, gEffectNode[nodeIndex].position.pva.value.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.value.z.median, gEffectNode[nodeIndex].position.pva.value.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].position.velocity = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.velocity.x.median, gEffectNode[nodeIndex].position.pva.velocity.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.velocity.y.median, gEffectNode[nodeIndex].position.pva.velocity.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.velocity.z.median, gEffectNode[nodeIndex].position.pva.velocity.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].position.acceleration = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.acceleration.x.median, gEffectNode[nodeIndex].position.pva.acceleration.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.acceleration.y.median, gEffectNode[nodeIndex].position.pva.acceleration.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].position.pva.acceleration.z.median, gEffectNode[nodeIndex].position.pva.acceleration.z.amplitude)
    //                    );
    //                    break;
    //                case SRT_TYPE_EASING:
    //                    break;
    //            }
    //            // Rotation
    //            switch (gEffectNode[nodeIndex].rotation.type) {
    //                case SRT_TYPE_STANDARD:
    //                    gEffectParticle[particleIndex].rotation.value = gEffectNode[nodeIndex].rotation.value;
    //                    break;
    //                case SRT_TYPE_PVA:
    //                    gEffectParticle[particleIndex].rotation.value = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.value.x.median, gEffectNode[nodeIndex].rotation.pva.value.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.value.y.median, gEffectNode[nodeIndex].rotation.pva.value.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.value.z.median, gEffectNode[nodeIndex].rotation.pva.value.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].rotation.velocity = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.velocity.x.median, gEffectNode[nodeIndex].rotation.pva.velocity.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.velocity.y.median, gEffectNode[nodeIndex].rotation.pva.velocity.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.velocity.z.median, gEffectNode[nodeIndex].rotation.pva.velocity.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].rotation.acceleration = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.acceleration.x.median, gEffectNode[nodeIndex].rotation.pva.acceleration.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.acceleration.y.median, gEffectNode[nodeIndex].rotation.pva.acceleration.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].rotation.pva.acceleration.z.median, gEffectNode[nodeIndex].rotation.pva.acceleration.z.amplitude)
    //                    );
    //                case SRT_TYPE_EASING:
    //                    break;
    //            }
    //            // Scale
    //            switch (gEffectNode[nodeIndex].scale.type) {
    //                case SRT_TYPE_STANDARD:
    //                    gEffectParticle[particleIndex].scale.value = gEffectNode[nodeIndex].scale.value;
    //                    break;
    //                case SRT_TYPE_PVA:
    //                    gEffectParticle[particleIndex].scale.value = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.value.x.median, gEffectNode[nodeIndex].scale.pva.value.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.value.y.median, gEffectNode[nodeIndex].scale.pva.value.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.value.z.median, gEffectNode[nodeIndex].scale.pva.value.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].scale.velocity = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.velocity.x.median, gEffectNode[nodeIndex].scale.pva.velocity.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.velocity.y.median, gEffectNode[nodeIndex].scale.pva.velocity.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.velocity.z.median, gEffectNode[nodeIndex].scale.pva.velocity.z.amplitude)
    //                    );
    //                    gEffectParticle[particleIndex].scale.acceleration = float3(
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.acceleration.x.median, gEffectNode[nodeIndex].scale.pva.acceleration.x.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.acceleration.y.median, gEffectNode[nodeIndex].scale.pva.acceleration.y.amplitude),
    //                        GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].scale.pva.acceleration.z.median, gEffectNode[nodeIndex].scale.pva.acceleration.z.amplitude)
    //                    );
    //                case SRT_TYPE_EASING:
    //                    break;
    //            }
    //            // Color
    //            switch (gEffectMesh[meshIndex].colorType) {
    //                case COLOR_TYPE_CONSTANT:
    //                    gEffectParticle[particleIndex].color = gEffectMesh[meshIndex].color;
    //                    break;
    //                case COLOR_TYPE_RANDOM:
    //                    gEffectParticle[particleIndex].color = float4(
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].randColor.minColor.r, gEffectMesh[meshIndex].randColor.maxColor.r),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].randColor.minColor.g, gEffectMesh[meshIndex].randColor.maxColor.g),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].randColor.minColor.b, gEffectMesh[meshIndex].randColor.maxColor.b),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].randColor.minColor.a, gEffectMesh[meshIndex].randColor.maxColor.a)
    //                    );
    //                    break;
    //                case COLOR_TYPE_EASING:
    //                    gEffectParticle[particleIndex].color = float4(
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].easingColor.startMinColor.r, gEffectMesh[meshIndex].easingColor.startMaxColor.r),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].easingColor.startMinColor.g, gEffectMesh[meshIndex].easingColor.startMaxColor.g),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].easingColor.startMinColor.b, gEffectMesh[meshIndex].easingColor.startMaxColor.b),
    //                        generator.GenerateInRange(gEffectMesh[meshIndex].easingColor.startMinColor.a, gEffectMesh[meshIndex].easingColor.startMaxColor.a)
    //                    );
    //                    break;
    //            }
    //            // LifeTime
    //            gEffectParticle[particleIndex].lifeTime = GenerateRandomInRange(generator.Generate1d(), gEffectNode[nodeIndex].common.lifeTime.median, gEffectNode[nodeIndex].common.lifeTime.amplitude);
                
    //            gEffectParticle[particleIndex].currentTime = 0.0f;
    //            gEffectParticle[particleIndex].isAlive = 1;
    //            gEffectParticle[particleIndex].nodeID = nodeIndex;
    //            gEffectParticle[particleIndex].meshID = meshIndex;
    //        }
    //    }
    //}
}