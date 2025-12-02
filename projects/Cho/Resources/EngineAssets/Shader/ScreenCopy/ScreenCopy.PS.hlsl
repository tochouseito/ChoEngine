#include "../header/ScreenCopy.hlsli"
Texture2D<float4> gTexure : register(t0);
SamplerState gSampler : register(s0);

PSOutput main(VSOutput input)
{
    PSOutput output;
    output.color = gTexure.Sample(gSampler, input.texcoord);
    output.color.a = 1.0f;
    return output;
}