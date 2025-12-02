#include "../header/ViewProjection.hlsli"
#include "../header/LineDraw.hlsli"

// ViewProjectionResource
ConstantBuffer<ViewProjection> g_ViewProjection : register(b0);

static float4x4 matWorld = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

PSInput main(VSInput input) {
    PSInput output;
    // 頂点インデックスに基づいて開始点または終了点を選択
    float3 position = input.position;
    // WorldViewProjection行列を作成
    float4x4 wvp = mul(g_ViewProjection.view, g_ViewProjection.projection);
    wvp = mul(matWorld, wvp);

    output.position = mul(float4(position, 1.0f), wvp);
    output.color = input.color;
    return output;
}