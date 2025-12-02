#include "../header/Particle.hlsli"
#include "../header/Material.hlsli"

// マテリアル
StructuredBuffer<Material> gIMaterial : register(t0, space0);
// Textures
Texture2D<float4> gTextures[] : register(t1, space0);
// Samplers
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VSOutput input) {
    PixelShaderOutput output;
    
    // 最終出力
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    // Material
    Material material = gIMaterial[input.materialID];
    // テクスチャカラー
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // alpha Test
    if(material.color.a <= 0.0f) {
        discard;
    }
    
    // テクスチャが有効ならテクスチャカラーを取得
    if (material.enableTexture != 0)
    {
        float2 texCoord = input.texcoord;
        if (material.uvFlipY)
        {
            texCoord.y = 1.0f - texCoord.y; // Y軸反転
        }
        // UV変換行列を適用
        float4 transformedUV = mul(float4(texCoord, 0.0f, 1.0f), material.matUV);
        textureColor = gTextures[material.textureID].Sample(gSampler, transformedUV.xy);
        // alpha Test
        if(textureColor.a <= 0.0f) {
            discard;
        }
    }
    // 合計
    finalColor.rgb =input.color.rgb * material.color.rgb * textureColor.rgb;
    finalColor.a = input.color.a * material.color.a * textureColor.a;
    // 最終出力
    output.color = finalColor;

    return output;
}