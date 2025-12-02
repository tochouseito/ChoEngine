#include "../header/Particle.hlsli"
#include "../header/Random.hlsli"

static const uint kMaxParticles = 1024;

// UAV: パーティクル
RWStructuredBuffer<Particle> gParticles : register(u0);
// SRV: エミッター
StructuredBuffer<EmitterSphere> gEmitter : register(t0);
// CBV: フレーム情報
ConstantBuffer<PerFrame> gPerFrame : register(b1);
// UAV: フリーリスト
ConsumeStructuredBuffer<uint> gFreeList : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    if (gEmitter[0].emit != 0) {
        RandomGenerator generator;
        generator.seed = (DTid + gPerFrame.time) * gPerFrame.time;
        
        for (uint countIndex = 0; countIndex < gEmitter[0].emitCount; ++countIndex) {
            // パーティクルごとにシードを変える
            generator.seed = generator.Generate3d() + float3(countIndex, countIndex * 2, countIndex * 3);
            // 戻り値で取得する
            uint particleIndex = gFreeList.Consume();
            // Append/Consume は空の時に自動でフェイルしないので、indexチェックが必要（オプション）
            if (particleIndex < kMaxParticles) {
                // position
                gParticles[particleIndex].position.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.value.x.median, gEmitter[0].position.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.value.y.median, gEmitter[0].position.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.value.z.median, gEmitter[0].position.value.z.amplitude)
                        );
                gParticles[particleIndex].position.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.velocity.x.median, gEmitter[0].position.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.velocity.y.median, gEmitter[0].position.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.velocity.z.median, gEmitter[0].position.velocity.z.amplitude)
                        );
                gParticles[particleIndex].position.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.acceleration.x.median, gEmitter[0].position.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.acceleration.y.median, gEmitter[0].position.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].position.acceleration.z.median, gEmitter[0].position.acceleration.z.amplitude)
                        );
                // rotation
                gParticles[particleIndex].rotation.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.value.x.median, gEmitter[0].rotation.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.value.y.median, gEmitter[0].rotation.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.value.z.median, gEmitter[0].rotation.value.z.amplitude)
                        );
                gParticles[particleIndex].rotation.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.velocity.x.median, gEmitter[0].rotation.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.velocity.y.median, gEmitter[0].rotation.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.velocity.z.median, gEmitter[0].rotation.velocity.z.amplitude)
                        );
                gParticles[particleIndex].rotation.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.acceleration.x.median, gEmitter[0].rotation.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.acceleration.y.median, gEmitter[0].rotation.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].rotation.acceleration.z.median, gEmitter[0].rotation.acceleration.z.amplitude)
                        );
                // scale
                gParticles[particleIndex].scale.value = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.value.x.median, gEmitter[0].scale.value.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.value.y.median, gEmitter[0].scale.value.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.value.z.median, gEmitter[0].scale.value.z.amplitude)
                        );
                gParticles[particleIndex].scale.velocity = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.velocity.x.median, gEmitter[0].scale.velocity.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.velocity.y.median, gEmitter[0].scale.velocity.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.velocity.z.median, gEmitter[0].scale.velocity.z.amplitude)
                        );
                gParticles[particleIndex].scale.acceleration = float3(
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.acceleration.x.median, gEmitter[0].scale.acceleration.x.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.acceleration.y.median, gEmitter[0].scale.acceleration.y.amplitude),
                            GenerateRandomInRange(generator.Generate1d(), gEmitter[0].scale.acceleration.z.median, gEmitter[0].scale.acceleration.z.amplitude)
                        );
                gParticles[particleIndex].color.rgb = float3(1.0f, 1.0f, 1.0f);
                gParticles[particleIndex].color.a = 1.0f;
                gParticles[particleIndex].isFadeOut = gEmitter[0].isFadeOut;
                gParticles[particleIndex].materialID = gEmitter[0].materialID;
                // LifeTime
                gParticles[particleIndex].lifeTime = GenerateRandomInRange(generator.Generate1d(), gEmitter[0].lifeTime.median, gEmitter[0].lifeTime.amplitude);
                gParticles[particleIndex].currentTime = 0.0f;
                gParticles[particleIndex].isAlive = true;
            }
            else {
                // カウンターが0ならConsumeしないようにする
                break; // 空きなし
            }
        }
    }
}
