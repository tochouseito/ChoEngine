#include "../header/ViewProjection.hlsli"
#include "../header/Billboard.hlsli"
#include "../header/Particle.hlsli"

// CBV: ビュープロジェクションリソース
ConstantBuffer<ViewProjection> gVP : register(b0);
// UAV: パーティクルリソース
RWStructuredBuffer<Particle> gParticles : register(u0);

struct VSInput {
    float4 pos : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
};

VSOutput main(VSInput input, uint instanceId : SV_InstanceID) {
    VSOutput output;
    Particle particle = gParticles[instanceId];
    // worldViewProjection
    float4x4 WVP = mul(gVP.view, gVP.projection);
    // cameraMatrix
    float4x4 cameraMatrix = gVP.matWorld;
    // cameraPosition
    float3 cameraPosition = gVP.cameraPosition;
    // scaleMatrix
    float4x4 scaleMatrix = ScaleMatrix(particle.scale.value);
    // translateMatrix
    float4x4 translateMatrix = TranslateMatrix(particle.position.value);
    // ビルボードがオンなら行列を作成する
    float4x4 rotateMatrix = (float4x4) 0;
    if (particle.isBillboard !=0)
    {
        rotateMatrix = BillboardMatrix(cameraMatrix);
    }
    else// 回転行列を作成
    {
        float3 radian = DegreesToRadians(particle.rotation.value);
        rotateMatrix = RotateXYZ(radian);
    }
    //worldMatrixを作成する
    float4x4 worldMatrix = mul(mul(scaleMatrix, rotateMatrix), translateMatrix);
    
    output.position = mul(input.pos, mul(worldMatrix, WVP));
    output.texcoord = input.uv;
    output.normal = normalize(input.normal);
    output.color = particle.color;
    output.materialID = particle.materialID;
    
    return output;
}