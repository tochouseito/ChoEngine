#include "../header/Demo.hlsli"

// トランスフォームリソース
ConstantBuffer<Transform> gTF : register(b0);

// ビュープロジェクションリソース
ConstantBuffer<ViewProjection> gVP : register(b1);

struct VertexShaderInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR;
    uint vertexID : VERTEXID;
};
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // worldViewProjection
    float4x4 WVP = mul(gVP.view, gVP.projection);
    WVP = mul(gTF.matWorld, WVP);
    
    // rootNode適用
    output.position = mul(input.position, mul(gTF.rootNode, WVP));

    // texcoord
    output.texcoord = input.texcoord;
    
    // 法線
    output.normal = normalize(mul(input.normal, (float3x3) gTF.worldInverse));
    
    output.worldPosition = mul(input.position, mul(gTF.matWorld, gTF.rootNode)).xyz;
    
    output.cameraPosition = gVP.cameraPosition;

    return output;
}