#include "../header/EffectRoot.hlsli"
#include "../header/EffectNode.hlsli"
#include "../header/EffectParticle.hlsli"
#include "../header/ViewProjection.hlsli"
#include "../header/Billboard.hlsli"

// CBV: ビュープロジェクションリソース
ConstantBuffer<ViewProjection> gVP : register(b0);
// SRV : Root 128個
StructuredBuffer<EffectRoot> gRoots : register(t0,space0);
// SRV : Node 1024個
StructuredBuffer<EffectNode> gNodes : register(t1, space0);
// SRV : Sprite 1024個
StructuredBuffer<EffectSprite> gSprites : register(t2);
// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0, space0);

VSOutput main(VSInput input, uint instanceId : SV_InstanceID, uint vertexId : SV_VertexID) {
    VSOutput output;
    
    EffectParticle particle = gParticles[instanceId];
    
    // Spriteじゃないなら何もしない
    if(particle.meshType != MESH_TYPE_SPRITE) {
        output.position = float4(0, 0, 0, 1);
        output.texcoord = float2(0, 0);
        output.normal = float3(0, 0, 0);
        output.color = float4(1, 1, 1, 0);
        output.instanceId = instanceId;
        return output;
    }
    
    EffectSprite sprite = gSprites[particle.meshID];
    
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
    switch (sprite.placement) {
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
    
    // 頂点座標タイプで頂点座標を変更
    float4 vertexPosition = input.position;
    
    if (sprite.VertexPositionType == VERTEX_TYPE_CONSTANT) {
        switch (input.vertexID) {
            case 0:
                vertexPosition.xy = sprite.vertexPosition.rightTop;
                break;
            case 1:
                vertexPosition.xy = sprite.vertexPosition.leftTop;
                break;
            case 2:
                vertexPosition.xy = sprite.vertexPosition.rightBottom;
                break;
            case 3:
                vertexPosition.xy = sprite.vertexPosition.leftBottom;
                break;
            case 4:
                vertexPosition.xy = sprite.vertexPosition.leftBottom;
                break;
            case 5:
                vertexPosition.xy = sprite.vertexPosition.rightBottom;
                break;
        }
    }
    
    output.position = mul(vertexPosition, mul(worldMatrix, WVP));
    output.texcoord = input.texcoord;
    output.normal = input.normal;
    output.color = particle.color;
    output.instanceId = instanceId;
    
    return output;
}