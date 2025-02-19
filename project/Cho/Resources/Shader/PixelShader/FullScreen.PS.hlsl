#include "../header/FullScreen.hlsli"

Texture2D<float4> gTexure : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexure.Sample(gSampler, input.texcoord);
    output.color.a=1.0f;
    return output;
}