#include "../header/EffectRoot.hlsli"
#include "../header/EffectNode.hlsli"
#include "../header/EffectParticle.hlsli"
#include "../header/ViewProjection.hlsli"
#include "../header/Billboard.hlsli"

// CBV: ビュープロジェクションリソース
ConstantBuffer<ViewProjection> gVP : register(b0);
// SRV : Root 128個
StructuredBuffer<EffectRoot> gRoots : register(t0, space0);
// SRV : Node 1024個
StructuredBuffer<EffectNode> gNodes : register(t1, space0);
// SRV : Ring 1024個
StructuredBuffer<EffectRing> gRing : register(t2);
// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0, space0);

VSOutput main(VSInput input, uint instanceId : SV_InstanceID, uint vertexId : SV_VertexID) {
    VSOutput output;
    
    EffectParticle particle = gParticles[instanceId];
    
    // Ringじゃないなら何もしない
    if (particle.meshType != MESH_TYPE_RING) {
        output.position = float4(0, 0, 0, 1);
        output.texcoord = float2(0, 0);
        output.normal = float3(0, 0, 0);
        output.color = float4(1, 1, 1, 0);
        output.instanceId = instanceId;
        return output;
    }
    
    EffectRing ring = gRing[particle.meshID];
    
    // 使わない頂点なら破棄
    if (vertexId >= ring.vertexCount * 4) {
        output.position = float4(0, 0, 0, 1);
        output.texcoord = float2(0, 0);
        output.normal = float3(0, 0, 0);
        output.color = float4(1, 1, 1, 0);
        output.instanceId = instanceId;
        return output;
    }
    
    // worldViewProjection
    float4x4 WVP = mul(gVP.view, gVP.projection);
    // cameraMatrix
    float4x4 cameraMatrix = gVP.matWorld;
    // cameraPosition
    float3 cameraPosition = gVP.cameraPosition;
    // scaleMatrix
    float4x4 scaleMatrix = ScaleMatrix(particle.wScl);
    // translateMatrix
    float4x4 translateMatrix = TranslateMatrix(particle.wPos);
    // rotationMatrix
    
    // billboardMatrix
    float4x4 billboardMatrix;
    switch (ring.placement) {
        case PLACEMENT_TYPE_BILLBOARD:{
                billboardMatrix = BillboardMatrix(cameraMatrix);
                break;
            }
        case PLACEMENT_TYPE_BILLBOARDY:{
                float3 radian = DegreesToRadians(particle.wRot);
                billboardMatrix = FixedYAxisBillboardMatrix(radian, particle.wPos, gVP.cameraPosition);
                break;
            }
        case PLACEMENT_TYPE_BILLBOARDXY:{
                float3 radian = DegreesToRadians(particle.wRot);
                //billboardMatrix = SphereBillboardMatrix(cameraMatrix, cameraPosition, particle.position.value);
                break;
            }
        case PLACEMENT_TYPE_CONSTANT:{
                float3 radian = DegreesToRadians(particle.wRot);
                billboardMatrix = RotateXYZ(radian);
                break;
            }
    }
    // worldMatrix
    float4x4 worldMatrix = mul(mul(scaleMatrix, billboardMatrix), translateMatrix);
    
    uint vertexCount = ring.vertexCount;
    const float kOuterRadius = 1.0f;
    const float kInnerRadius = 0.5f;
    const float radianPerVertex = 2.0f * PI / float(vertexCount);
    uint segmentIndex = vertexId / 4;
    uint cornerIndex = vertexId % 4;

    float currentAngle = radianPerVertex * float(segmentIndex);
    float nextAngle = radianPerVertex * float((segmentIndex + 1) % vertexCount); // wrap-around

    float uCurrent = float(segmentIndex) / float(vertexCount);
    float uNext = float(segmentIndex + 1) / float(vertexCount);
    if (segmentIndex == vertexCount - 1)
        uNext = 1.0f;

    float3 posLocal;
    float2 uv;

    switch (cornerIndex) {
        case 0: // outer current
            posLocal = float3(cos(currentAngle) * kOuterRadius, sin(currentAngle) * kOuterRadius, 0);
            uv = float2(uCurrent, 0.0f);
            break;
        case 1: // outer next
            posLocal = float3(cos(nextAngle) * kOuterRadius, sin(nextAngle) * kOuterRadius, 0);
            uv = float2(uNext, 0.0f);
            break;
        case 2: // inner current
            posLocal = float3(cos(currentAngle) * kInnerRadius, sin(currentAngle) * kInnerRadius, 0);
            uv = float2(uCurrent, 1.0f);
            break;
        case 3: // inner next
            posLocal = float3(cos(nextAngle) * kInnerRadius, sin(nextAngle) * kInnerRadius, 0);
            uv = float2(uNext, 1.0f);
            break;
    }

    float4 worldPos = mul(float4(posLocal, 1.0f), worldMatrix);
    output.position = mul(worldPos, WVP);
    output.texcoord = input.texcoord;
    output.normal = input.normal;
    output.color = particle.color;
    output.instanceId = instanceId;
    
    return output;
}