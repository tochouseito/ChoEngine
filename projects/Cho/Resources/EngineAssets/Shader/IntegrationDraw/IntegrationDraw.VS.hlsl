#include "../header/Demo.hlsli"
#include "../header/Skinning.hlsli"

// ViewProjectionResource
ConstantBuffer<ViewProjection> gVP : register(b0);
// IntegrationTransformResource
StructuredBuffer<Transform> gITF : register(t0, space0);
// UseTransformMapIndexResource
StructuredBuffer<uint> gUseIndex : register(t1, space0);
// BoneMatrixResource
StructuredBuffer<Well> gBoneMatrix : register(t2, space0);
// SkinningInfluenceResource
StructuredBuffer<VertexInfluence> gSkinningInfluence : register(t3, space0);
// SkinningInfoResource
ConstantBuffer<SkinningInformation> gSkinningInfo : register(b1);

struct VSIn {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
};
VSOut main(VSIn input, uint instanceId : SV_InstanceID) {
    VSOut output;
    
    uint index = gUseIndex[instanceId];
    Transform tf = gITF[index];
    
    VertexInfluence influence = gSkinningInfluence[input.vertexID];
    float4 position = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    
    if (tf.isAnimated != 0&&gSkinningInfo.isSkinned != 0&&input.vertexID<gSkinningInfo.numVertices) {
        // スキニングを行う
        float offset = gSkinningInfo.boneCount * tf.isBoneOffsetStartIndex;
        position = mul(input.position, gBoneMatrix[influence.index.x + offset].skeletonSpaceMatrix) * influence.weight.x;
        position += mul(input.position, gBoneMatrix[influence.index.y + offset].skeletonSpaceMatrix) * influence.weight.y;
        position += mul(input.position, gBoneMatrix[influence.index.z + offset].skeletonSpaceMatrix) * influence.weight.z;
        position += mul(input.position, gBoneMatrix[influence.index.w + offset].skeletonSpaceMatrix) * influence.weight.w;
        position.w = 1.0f; // スキニング後の位置はw成分を1にする必要がある
        // 法線もスキニング
        normal = mul(input.normal, (float3x3) gBoneMatrix[influence.index.x + offset].skeletonSpaceInverseTransposeMatrix) * influence.weight.x;
        normal += mul(input.normal, (float3x3) gBoneMatrix[influence.index.y + offset].skeletonSpaceInverseTransposeMatrix) * influence.weight.y;
        normal += mul(input.normal, (float3x3) gBoneMatrix[influence.index.z + offset].skeletonSpaceInverseTransposeMatrix) * influence.weight.z;
        normal += mul(input.normal, (float3x3) gBoneMatrix[influence.index.w + offset].skeletonSpaceInverseTransposeMatrix) * influence.weight.w;
        normal = normalize(normal); // 法線は正規化する必要がある
    }else {
        position = input.position;
        normal = input.normal;
    }
    
    // worldViewProjection
        float4x4 WVP = mul(gVP.view, gVP.projection);
    WVP = mul(tf.matWorld, WVP);
    // rootNode適用
    output.position = mul(position, mul(tf.rootNode, WVP));
    // texcoord
    output.texcoord = input.texcoord;
    // normal
    output.normal = normalize(mul(normal, (float3x3)tf.worldInverse));
    //
    output.worldPosition = mul(input.position, mul(tf.matWorld, tf.rootNode)).xyz;
    //
    output.cameraPosition = gVP.cameraPosition;
    // 
    output.materialID = tf.materialID;
    //
    output.color = input.color;

    return output;
}