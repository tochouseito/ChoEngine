#include "../header/ViewProjection.hlsli"
#include "../header/Skybox.hlsli"
#include "../header/Math.hlsli"

struct VSInput
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

// CBV : ViewProjection
ConstantBuffer<ViewProjection> gVP : register(b0);

VSOutput main(VSInput input)
{
    VSOutput output;
    // worldViewProjection
    // viewの平行移動成分を無視
    float4x4 view = gVP.view;
    view[0][0] = 0.0f;
    view[0][1] = 0.0f;
    view[0][2] = 0.0f;
    view[0][3] = 1.0f;
    float4x4 WVP = mul(gVP.view, gVP.projection);
    // TranslateMatrix
    float4x4 translateMatrix = TranslateMatrix(gVP.cameraPosition);
    output.position = mul(input.position, mul(translateMatrix, WVP)).xyww;
    output.texCoord = input.position.xyz;
    return output;
}