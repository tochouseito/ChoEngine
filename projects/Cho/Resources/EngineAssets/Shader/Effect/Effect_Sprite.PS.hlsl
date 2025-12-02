#include "../header/EffectRoot.hlsli"
#include "../header/EffectNode.hlsli"
#include "../header/EffectParticle.hlsli"

// SRV : Node 1024個
StructuredBuffer<EffectNode> gNodes : register(t0, space1);
// UAV : Particle 128x1024個
RWStructuredBuffer<EffectParticle> gParticles : register(u0, space1);
// Textures
Texture2D<float4> gTextures[] : register(t1, space1);
// Samplers
SamplerState gSampler : register(s0);

struct PSOutput {
    float4 color : SV_TARGET0;
};

PSOutput main(VSOutput input) {
    PSOutput output;
	
    EffectParticle particle = gParticles[input.instanceId];
    EffectNode node = gNodes[particle.nodeID];
    float4 textureColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (node.drawCommon.textureID == 0) {
        output.color = input.color;
    }
    else {
        textureColor = gTextures[node.drawCommon.textureID].Sample(gSampler, input.texcoord);
        output.color.rgb = input.color.rgb * textureColor.rgb;
        output.color.a = input.color.a * textureColor.a;
    }
    // alpha test
    if (output.color.a <= 0.0f) {
        discard;
    }
    return output;
}