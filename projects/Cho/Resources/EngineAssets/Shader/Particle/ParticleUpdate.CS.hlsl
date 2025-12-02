#include "../header/Particle.hlsli"

static const uint kMaxParticles = 1024;

// UAV: パーティクルリソース
RWStructuredBuffer<Particle> gParticles : register(u0);
// CBV: PerFrameリソース
ConstantBuffer<PerFrame> gPerFrame : register(b0);
// UAV: フリーリスト
AppendStructuredBuffer<uint> gFreeList : register(u1);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;
    // パーティクルが死んでいるなら何もしない
    if (gParticles[particleIndex].isAlive == 0 || particleIndex >= kMaxParticles)
    {
        return;
    }
    // 経過時間が生存時間を超えたら死ぬ
    if (gParticles[particleIndex].currentTime >= gParticles[particleIndex].lifeTime)
    {
        gParticles[particleIndex] = (Particle) 0; // パーティクルを初期化
        // フリーリストに追加
        gFreeList.Append(particleIndex);
        return;
    }
    // パーティクルが生きているなら更新する
    // 位置
    gParticles[particleIndex].position.value += gParticles[particleIndex].position.velocity * gPerFrame.deltaTime;
    // 速度
    gParticles[particleIndex].position.velocity += gParticles[particleIndex].position.acceleration * gPerFrame.deltaTime;
    // 回転
    gParticles[particleIndex].rotation.value += gParticles[particleIndex].rotation.velocity * gPerFrame.deltaTime;
    // 回転速度
    gParticles[particleIndex].rotation.velocity += gParticles[particleIndex].rotation.acceleration * gPerFrame.deltaTime;
    // スケール
    gParticles[particleIndex].scale.value += gParticles[particleIndex].scale.velocity * gPerFrame.deltaTime;
    // スケール速度
    gParticles[particleIndex].scale.velocity += gParticles[particleIndex].scale.acceleration * gPerFrame.deltaTime;
    // 色
    // フェードアウトフラグがオンなら、アルファでフェードアウト
    if (gParticles[particleIndex].isFadeOut != 0)
    {
        float alpha = 1.0f - (gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
        gParticles[particleIndex].color.a = saturate(alpha);
    }
    // 経過時間更新
    gParticles[particleIndex].currentTime++;
}
