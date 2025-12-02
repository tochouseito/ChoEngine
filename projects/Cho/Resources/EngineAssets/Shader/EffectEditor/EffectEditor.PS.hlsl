#include "../header/Effect.hlsli"

// SRV : EffectNode
StructuredBuffer<EffectNode> gEffectNode : register(t0);
// UAV : EffectParticle 128x1024個
RWStructuredBuffer<EffectParticle> gEffectParticle : register(u0);

// Textures
Texture2D<float4> gTextures[] : register(t1);
// Samplers
SamplerState gSampler : register(s0);

struct PSOutput {
    float4 color : SV_TARGET0;
};

PSOutput main(VSOutput input) {
    PSOutput output;
	
    EffectParticle particle = gEffectParticle[input.instanceId];
    EffectNode node = gEffectNode[particle.nodeID];
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (node.drawCommon.textureID == 0)
    {
        output.color = input.color;
    }
    else
    {
        textureColor = gTextures[node.drawCommon.textureID].Sample(gSampler, input.texcoord);
        output.color = input.color * textureColor;
    }
    return output;
}